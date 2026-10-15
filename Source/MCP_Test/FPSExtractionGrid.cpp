#include "FPSExtractionSubsystem.h"
#include "FPSExtractionMode.h"
#include "Kismet/GameplayStatics.h"

namespace
{
FIntPoint Dimensions(int32 Capacity)
{
    return Capacity == 64 ? FIntPoint(8,8) : Capacity == 20 ? FIntPoint(5,4) : FIntPoint::ZeroValue;
}
bool Overlaps(int32 X, int32 Y, int32 W, int32 H, int32 OtherX, int32 OtherY, int32 OtherW, int32 OtherH)
{
    return X < OtherX + OtherW && X + W > OtherX && Y < OtherY + OtherH && Y + H > OtherY;
}
}

int32 UFPSExtractionSubsystem::Count(const TArray<FFPSExtractionStack>& Inventory, FName Id) const
{
    int64 Total=0;
    for(const auto& S:Inventory) if(S.ItemId==Id) Total+=S.Quantity;
    return int32(FMath::Clamp<int64>(Total,0,MAX_int32));
}
int32 UFPSExtractionSubsystem::SlotsUsed(const TArray<FFPSExtractionStack>& Inventory) const
{
    int64 Total=0;
    for(const auto& S:Inventory)
    {
        const auto* I=FindItem(S.ItemId);
        if(!I || S.Quantity<=0 || S.Quantity>I->StackLimit) return MAX_int32;
        Total+=int64(I->Width)*I->Height;
    }
    return int32(FMath::Min<int64>(Total,MAX_int32));
}
bool UFPSExtractionSubsystem::CanPlace(const TArray<FFPSExtractionStack>& Inventory,FName Id,int32 X,int32 Y,bool Rotated,int32 Capacity) const
{
    const auto* I=FindItem(Id); const auto Size=Dimensions(Capacity);
    if(!I || X<0 || Y<0) return false;
    const int32 W=Rotated?I->Height:I->Width, H=Rotated?I->Width:I->Height;
    if(W<=0 || H<=0 || X>Size.X-W || Y>Size.Y-H) return false;
    for(const auto& S:Inventory)
    {
        const auto* Other=FindItem(S.ItemId);
        if(!Other || Overlaps(X,Y,W,H,S.X,S.Y,S.bRotated?Other->Height:Other->Width,S.bRotated?Other->Width:Other->Height)) return false;
    }
    return true;
}
bool UFPSExtractionSubsystem::ValidateGrid(const TArray<FFPSExtractionStack>& Inventory,int32 Capacity) const
{
    if(Inventory.Num()>Capacity || SlotsUsed(Inventory)>Capacity) return false;
    TArray<FFPSExtractionStack> Accepted; TSet<FGuid> Ids;
    for(const auto& S:Inventory)
    {
        if(!S.StackId.IsValid() || Ids.Contains(S.StackId) || !CanPlace(Accepted,S.ItemId,S.X,S.Y,S.bRotated,Capacity)) return false;
        Ids.Add(S.StackId); Accepted.Add(S);
    }
    return true;
}
bool UFPSExtractionSubsystem::TryAdd(TArray<FFPSExtractionStack>& Inventory,FName Id,int32 Quantity,int32 Capacity) const
{
    const auto* I=FindItem(Id); const auto Size=Dimensions(Capacity);
    if(!I || Quantity<=0 || Size.X<=0 || Quantity>int64(Capacity)*I->StackLimit || !ValidateGrid(Inventory,Capacity)) return false;
    auto Copy=Inventory;
    for(auto& S:Copy) if(S.ItemId==Id)
    {
        const int32 N=FMath::Min(Quantity,I->StackLimit-S.Quantity); S.Quantity+=N; Quantity-=N;
    }
    while(Quantity>0)
    {
        bool Found=false;
        for(int32 Rotation=0;Rotation<2 && !Found;++Rotation)
            for(int32 Y=0;Y<Size.Y && !Found;++Y)
                for(int32 X=0;X<Size.X && !Found;++X)
                    if(CanPlace(Copy,Id,X,Y,Rotation!=0,Capacity))
                    {
                        FFPSExtractionStack S; S.ItemId=Id; S.Quantity=FMath::Min(Quantity,I->StackLimit);
                        S.StackId=FGuid::NewGuid(); S.X=X; S.Y=Y; S.bRotated=Rotation!=0;
                        Copy.Add(S); Quantity-=S.Quantity; Found=true;
                    }
        if(!Found) return false;
    }
    Inventory=MoveTemp(Copy); return true;
}
bool UFPSExtractionSubsystem::TryRemove(TArray<FFPSExtractionStack>& Inventory,FName Id,int32 Quantity) const
{
    if(Quantity<=0 || Count(Inventory,Id)<Quantity) return false;
    for(auto& S:Inventory) if(S.ItemId==Id)
    { const int32 N=FMath::Min(S.Quantity,Quantity); S.Quantity-=N; Quantity-=N; }
    Inventory.RemoveAll([](const auto& S){return S.Quantity==0;}); return true;
}
bool UFPSExtractionSubsystem::MoveStack(EFPSExtractionContainer From,FGuid Id,EFPSExtractionContainer To,int32 X,int32 Y,bool Rotated)
{ return MutateGrid(From,Id,To,X,Y,Rotated,false); }
bool UFPSExtractionSubsystem::AutoTransferStack(EFPSExtractionContainer From,FGuid Id,EFPSExtractionContainer To)
{ return MutateGrid(From,Id,To,0,0,false,true); }
bool UFPSExtractionSubsystem::MutateGrid(EFPSExtractionContainer From,FGuid Id,EFPSExtractionContainer To,int32 X,int32 Y,bool Rotated,bool Auto)
{
    auto ValidContainer=[](EFPSExtractionContainer V){return V==EFPSExtractionContainer::Warehouse || V==EFPSExtractionContainer::Backpack;};
    if(!IsAuthorized() || !bStorageHealthy || !Profile || !ValidContainer(From) || !ValidContainer(To) ||
        (Phase!=EFPSExtractionPhase::Base && Phase!=EFPSExtractionPhase::Raid)) return Fail(TEXT("当前不能整理物资。"));
    const bool Raid=Phase==EFPSExtractionPhase::Raid;
    if(Raid && (From!=EFPSExtractionContainer::Backpack || To!=EFPSExtractionContainer::Backpack)) return Fail(TEXT("行动中无法访问基地仓库。"));
    auto* Candidate=Raid?nullptr:CopyProfile();
    auto Bag=Raid?Backpack:Candidate->PreparedBackpack;
    auto Store=Raid?TArray<FFPSExtractionStack>():Candidate->Warehouse;
    auto& Source=From==EFPSExtractionContainer::Backpack?Bag:Store;
    auto& Target=To==EFPSExtractionContainer::Backpack?Bag:Store;
    const int32 Index=Source.IndexOfByPredicate([Id](const auto& S){return S.StackId==Id;});
    if(Index==INDEX_NONE) return Fail(TEXT("物品已移动或不在该容器中。"));
    auto Stack=Source[Index]; Source.RemoveAt(Index);
    const int32 Capacity=To==EFPSExtractionContainer::Backpack?20:64;
    const int32 OriginalQuantity=Stack.Quantity;
    int32 MovedQuantity=OriginalQuantity;
    if(Auto)
    {
        // Preserve the physical ID when a whole stack occupies a new slot. Merging
        // into existing stacks retains their IDs, just like a manual drop.
        TSet<FGuid> Existing;
        for(const auto& S:Target)Existing.Add(S.StackId);
        if(!TryAdd(Target,Stack.ItemId,Stack.Quantity,Capacity)) return Fail(TEXT("目标容器没有可容纳物品的连续空间。"));
        for(auto& S:Target)if(!Existing.Contains(S.StackId)){S.StackId=Stack.StackId;break;}
    }
    else
    {
        const auto* Item=FindItem(Stack.ItemId);
        const auto* Under=Target.FindByPredicate([&](const auto& S)
        {
            const auto* Other=FindItem(S.ItemId);
            return Other && X>=S.X && Y>=S.Y && X<S.X+(S.bRotated?Other->Height:Other->Width) && Y<S.Y+(S.bRotated?Other->Width:Other->Height);
        });
        if(Under && Under->ItemId==Stack.ItemId && Item)
        {
            MovedQuantity=FMath::Min(Stack.Quantity,Item->StackLimit-Under->Quantity);
            if(MovedQuantity<=0)return Fail(TEXT("目标堆叠已满，物资留在原处。"));
            const FGuid TargetId=Under->StackId;
            for(auto& S:Target)if(S.StackId==TargetId){S.Quantity+=MovedQuantity;break;}
            Stack.Quantity-=MovedQuantity;
            // Source and Target may alias. Reinsert only after finishing the target
            // mutation, retaining the remainder's original ID, position and rotation.
            if(Stack.Quantity>0)Source.Insert(Stack,Index);
        }
        else
        {
            if(!CanPlace(Target,Stack.ItemId,X,Y,Rotated,Capacity)) return Fail(TEXT("无法放置：物品越界或与其他物品重叠。"));
            Stack.X=X; Stack.Y=Y; Stack.bRotated=Rotated; Target.Add(Stack);
        }
    }
    if(!ValidateGrid(Bag,20) || (!Raid && !ValidateGrid(Store,64))) return Fail(TEXT("容器状态无效，操作未提交。"));
    if(Raid) Backpack=MoveTemp(Bag);
    else
    {
        Candidate->PreparedBackpack=MoveTemp(Bag); Candidate->Warehouse=MoveTemp(Store);
        if(!Commit(Candidate)) return false;
    }
    Feedback=MovedQuantity<OriginalQuantity?FString::Printf(TEXT("已合并 %d 件，余下 %d 件保留原位。"),MovedQuantity,OriginalQuantity-MovedQuantity):TEXT("物资已整理。");return true;
}
bool UFPSExtractionSubsystem::SplitStack(EFPSExtractionContainer Container,FGuid Id,int32 Quantity)
{
    if(!IsAuthorized() || !bStorageHealthy || !Profile ||
        (Container!=EFPSExtractionContainer::Warehouse && Container!=EFPSExtractionContainer::Backpack) ||
        (Phase!=EFPSExtractionPhase::Base && Phase!=EFPSExtractionPhase::Raid))return Fail(TEXT("当前不能拆分物资。"));
    const bool Raid=Phase==EFPSExtractionPhase::Raid;
    if(Raid && Container!=EFPSExtractionContainer::Backpack)return Fail(TEXT("行动中无法访问基地仓库。"));
    auto* Candidate=Raid?nullptr:CopyProfile();
    auto Inventory=Raid?Backpack:Container==EFPSExtractionContainer::Warehouse?Candidate->Warehouse:Candidate->PreparedBackpack;
    const int32 Index=Inventory.IndexOfByPredicate([Id](const auto& S){return S.StackId==Id;});
    if(Index==INDEX_NONE)return Fail(TEXT("物品已移动或不在该容器中。"));
    if(Quantity<=0 || Quantity>=Inventory[Index].Quantity)return Fail(TEXT("拆分数量须大于 0 且小于当前数量。"));
    auto NewStack=Inventory[Index];NewStack.Quantity=Quantity;NewStack.StackId=FGuid::NewGuid();
    const int32 Capacity=Container==EFPSExtractionContainer::Warehouse?64:20;
    const auto Size=Dimensions(Capacity);
    bool Placed=false;
    for(int32 Rotation=0;Rotation<2 && !Placed;++Rotation)
        for(int32 Y=0;Y<Size.Y && !Placed;++Y)
            for(int32 X=0;X<Size.X && !Placed;++X)
            {
                const bool Rotated=Rotation==0?NewStack.bRotated:!NewStack.bRotated;
                if(CanPlace(Inventory,NewStack.ItemId,X,Y,Rotated,Capacity))
                {NewStack.X=X;NewStack.Y=Y;NewStack.bRotated=Rotated;Placed=true;}
            }
    if(!Placed)return Fail(TEXT("没有可容纳新堆叠的连续空间，拆分未提交。"));
    Inventory[Index].Quantity-=Quantity;Inventory.Add(NewStack);
    if(!ValidateGrid(Inventory,Capacity))return Fail(TEXT("容器状态无效，拆分未提交。"));
    if(Raid)Backpack=MoveTemp(Inventory);
    else
    {
        (Container==EFPSExtractionContainer::Warehouse?Candidate->Warehouse:Candidate->PreparedBackpack)=MoveTemp(Inventory);
        if(!Commit(Candidate))return false;
    }
    Feedback=FString::Printf(TEXT("已拆出 %s ×%d，放在当前容器空位。"),*ItemName(NewStack.ItemId).ToString(),Quantity);return true;
}
bool UFPSExtractionSubsystem::SellStack(FGuid Id)
{
    if(!IsBase()) return Fail(TEXT("请在基地出售仓库物品。"));
    auto* C=CopyProfile(); const int32 Index=C->Warehouse.IndexOfByPredicate([Id](const auto& S){return S.StackId==Id;});
    if(Index==INDEX_NONE) return Fail(TEXT("仓库中未找到该物品。"));
    const auto Stack=C->Warehouse[Index]; const auto* I=FindItem(Stack.ItemId);
    if(!I || int64(C->Credits)+int64(Stack.Quantity)*I->SellPrice>MAX_int32) return Fail(TEXT("出售金额超出范围。"));
    C->Credits+=Stack.Quantity*I->SellPrice; C->Warehouse.RemoveAt(Index);
    if(!Commit(C)) return false;
    Feedback=TEXT("所选物品已出售。" ); return true;
}
bool UFPSExtractionSubsystem::DropStack(FGuid Id)
{
    if(!IsAuthorized() || Phase!=EFPSExtractionPhase::Raid) return Fail(TEXT("只能在行动中丢弃背包物品。"));
    const int32 Index=Backpack.IndexOfByPredicate([Id](const auto& S){return S.StackId==Id;});
    if(Index==INDEX_NONE) return Fail(TEXT("背包中未找到该物品。"));
    const auto Stack=Backpack[Index];
    auto* PC=Cast<AFPSExtractionController>(UGameplayStatics::GetPlayerController(this,0));
    auto* Mode=GetWorld()->GetAuthGameMode<AFPSExtractionMode>();
    if(!PC || !Mode || !Mode->DropCargo(PC,Stack.ItemId,Stack.Quantity)) return Fail(TEXT("无法放置地面物资。"));
    Backpack.RemoveAt(Index); Feedback=TEXT("整组物品已放回地面。" ); return true;
}
bool UFPSExtractionSubsystem::UpgradeProfile(UFPSExtractionSave* C) const
{
    if(!C || C->Version!=1) return false;
    TArray<FFPSExtractionStack> Packed;
    TSet<FName> Seen;
    // Old saves aggregate by item. Repack a copy; a failed migration keeps original files untouched.
    for(const auto& S:C->Warehouse)
    {
        if(Seen.Contains(S.ItemId) || !TryAdd(Packed,S.ItemId,S.Quantity,64)) return false;
        Seen.Add(S.ItemId);
    }
    C->Warehouse=MoveTemp(Packed); C->PreparedBackpack.Reset(); C->Version=2;
    return true;
}
