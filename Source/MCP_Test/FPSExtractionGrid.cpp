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
    if(Auto)
    {
        if(!TryAdd(Target,Stack.ItemId,Stack.Quantity,Capacity)) return Fail(TEXT("目标容器没有可容纳物品的连续空间。"));
    }
    else
    {
        const auto* Item=FindItem(Stack.ItemId);
        const auto* Under=Target.FindByPredicate([&](const auto& S)
        {
            const auto* Other=FindItem(S.ItemId);
            return Other && X>=S.X && Y>=S.Y && X<S.X+(S.bRotated?Other->Height:Other->Width) && Y<S.Y+(S.bRotated?Other->Width:Other->Height);
        });
        if(Under && Under->ItemId==Stack.ItemId && Item && Stack.Quantity<=Item->StackLimit-Under->Quantity)
        {
            for(auto& S:Target) if(S.StackId==Under->StackId) {S.Quantity+=Stack.Quantity; break;}
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
    Feedback=TEXT("物资已整理。" ); return true;
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
