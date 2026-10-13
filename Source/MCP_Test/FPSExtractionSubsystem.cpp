#include "FPSExtractionSubsystem.h"
#include "FPSExtractionMode.h"
#include "FPSAttachmentData.h"
#include "FPSDemoCharacter.h"
#include "FPSWeaponComponent.h"
#include "FPSDemoGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"

namespace
{
constexpr int32 BagCapacity = 20, StoreCapacity = 64;
const TCHAR* LabMap = TEXT("/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing");
void ClearRuntime(FFPSWeaponLoadout& L)
{
    L.Action = EFPSWeaponAction::Idle; L.ActionToken.Invalidate(); L.ActionEndsAt = L.ActionStartedAt = L.EquipStartedAt = 0;
    L.bAiming = L.bFireActive = false; L.FirePredictionKey = 0;
    for (auto* Gun : {&L.Primary, &L.Secondary})
    { Gun->NextShotAt = 0; Gun->LastShotAt = -100; Gun->RecoilIndex = 0; }
}
}
void UFPSExtractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UFPSWeaponCatalog>();
    SlotPrefix = TEXT("ExtractionLab");
    FString Override;
    FParse::Value(FCommandLine::Get(), TEXT("ExtractionProfile="), Override);
    bool SafeName = !Override.IsEmpty() && Override.Len() <= 64;
    for (TCHAR C : Override) SafeName &= FChar::IsAlnum(C) || C == TEXT('_') || C == TEXT('-');
    if (SafeName)
        SlotPrefix = TEXT("ExtractionLab_") + Override;
    CreateCatalog();
}
void UFPSExtractionSubsystem::CreateCatalog()
{
    auto Item = [this](const TCHAR* Id, const TCHAR* Name, const TCHAR* Description, int32 Price, const TCHAR* Attachment = TEXT(""), int32 Width = 1, int32 Height = 1)
    {
        FFPSExtractionItem I; I.ItemId = Id; I.Name = FText::FromString(Name); I.Description = FText::FromString(Description);
        I.SellPrice = Price; I.AttachmentId = Attachment; I.StackLimit = I.AttachmentId.IsNone() ? 20 : 1; I.Width = Width; I.Height = Height; Items.Add(I);
    };
    Item(TEXT("Scrap"), TEXT("废金属"), TEXT("可出售，或加工为机械零件。"), 3, TEXT(""), 2, 1);
    Item(TEXT("Wire"), TEXT("铜线"), TEXT("用于制作枪械配件。"), 8);
    Item(TEXT("Parts"), TEXT("机械零件"), TEXT("由废金属加工，是配件制作的基础材料。"), 12, TEXT(""), 2, 1);
    Item(TEXT("Optic"), TEXT("M4 固定倍率瞄具"), TEXT("改变瞄准视野；仅适用于兼容 M4 接口。"), 55, TEXT("Attachment.M4_OpticFixed"), 2, 1);
    Item(TEXT("Brake"), TEXT("M4 制退器"), TEXT("降低后坐力。"), 45, TEXT("Attachment.M4_MuzzleBrake"));
    Item(TEXT("Magazine"), TEXT("M4 扩容弹匣"), TEXT("提高容量，换弹耗时增加。"), 60, TEXT("Attachment.M4_MagazineExtended"), 1, 2);
    auto Recipe = [this](const TCHAR* Id, const TCHAR* Name, FName Input, int32 Amount, FName Output, int32 Count, float Seconds, int32 Cost, bool Wire)
    {
        FFPSExtractionRecipe R; R.RecipeId = Id; R.Name = FText::FromString(Name); R.Inputs.Add({Input, Amount});
        if (Wire) R.Inputs.Add({TEXT("Wire"), 1});
        R.Output = {Output, Count}; R.Duration = Seconds; R.CreditCost = Cost; Recipes.Add(R);
    };
    Recipe(TEXT("ProcessParts"), TEXT("加工机械零件"), TEXT("Scrap"), 4, TEXT("Parts"), 2, 5, 0, false);
    Recipe(TEXT("CraftBrake"), TEXT("制作 M4 制退器"), TEXT("Parts"), 2, TEXT("Brake"), 1, 8, 5, true);
    Recipe(TEXT("CraftOptic"), TEXT("制作 M4 瞄具"), TEXT("Parts"), 3, TEXT("Optic"), 1, 10, 8, true);
    Recipe(TEXT("CraftMagazine"), TEXT("制作 M4 扩容弹匣"), TEXT("Parts"), 3, TEXT("Magazine"), 1, 10, 8, true);
}
const FFPSExtractionItem* UFPSExtractionSubsystem::FindItem(FName Id) const { return Items.FindByPredicate([Id](const auto& I) { return I.ItemId == Id; }); }
const FFPSExtractionRecipe* UFPSExtractionSubsystem::FindRecipe(FName Id) const { return Recipes.FindByPredicate([Id](const auto& R) { return R.RecipeId == Id; }); }
FText UFPSExtractionSubsystem::ItemName(FName Id) const { const auto* I = FindItem(Id); return I ? I->Name : FText::FromName(Id); }
bool UFPSExtractionSubsystem::IsAuthorized() const
{
    return GetWorld() && GetWorld()->GetNetMode() == NM_Standalone && GetWorld()->GetAuthGameMode<AFPSExtractionMode>();
}
bool UFPSExtractionSubsystem::IsBase() const { return IsAuthorized() && bStorageHealthy && Profile && Phase == EFPSExtractionPhase::Base; }
bool UFPSExtractionSubsystem::Fail(const FString& Text) { Feedback = Text; return false; }
UFPSExtractionSave* UFPSExtractionSubsystem::CopyProfile() const { return Profile ? DuplicateObject<UFPSExtractionSave>(Profile, const_cast<UFPSExtractionSubsystem*>(this)) : nullptr; }
bool UFPSExtractionSubsystem::ValidateProfile(const UFPSExtractionSave* C) const
{
    if (!C || C->Version != 2 || C->Revision < 0 || C->Credits < 0 || !ValidateGrid(C->Warehouse, StoreCapacity) || !ValidateGrid(C->PreparedBackpack, BagCapacity)) return false;
    TSet<FGuid> Seen;
    for (const auto& S : C->Warehouse) Seen.Add(S.StackId);
    for (const auto& S : C->PreparedBackpack) if (Seen.Contains(S.StackId)) return false;
    if (!C->ProductionRecipe.IsNone() && (!FindRecipe(C->ProductionRecipe) || C->ProductionEnds.GetTicks() <= 0)) return false;
    const auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    for (const auto* Gun : {&C->Loadout.Primary, &C->Loadout.Secondary})
    {
        FFPSResolvedWeaponStats Stats; TArray<FString> Errors;
        if (!Gun->IsValid() || Gun->Magazine < 0 || Gun->Reserve < 0 || Gun->ConfigurationRevision == 0 ||
            !Catalog->Resolve(Gun->WeaponId, Gun->AttachmentIds, Stats, Errors) || Gun->Magazine > Stats.MagazineSize) return false;
    }
    return C->Loadout.Primary.WeaponId == TEXT("Weapon.M4") && C->Loadout.Secondary.WeaponId == TEXT("Weapon.M9") &&
        C->Loadout.Primary.InstanceId != C->Loadout.Secondary.InstanceId;
}
bool UFPSExtractionSubsystem::Commit(UFPSExtractionSave* Candidate)
{
    if (!IsAuthorized() || !Candidate || !bStorageHealthy || !ValidateProfile(Candidate) || Candidate->Revision == MAX_int32)
        return Fail(TEXT("存档状态无效，操作未提交。"));
    ++Candidate->Revision;
    // Alternate snapshots: a failed write cannot erase the preceding committed state.
    const FString Slot = SlotPrefix + FString::Printf(TEXT("_%d"), Candidate->Revision % 2);
    if (!UGameplayStatics::SaveGameToSlot(Candidate, Slot, 0)) return Fail(TEXT("保存失败；库存与装备未改变，请检查磁盘后重试。"));
    Profile = Candidate;
    UE_LOG(LogTemp, Display, TEXT("FPS_EXTRACTION SAVE revision=%d credits=%d active=%d"), Profile->Revision, Profile->Credits, Profile->bExpeditionActive);
    return true;
}
bool UFPSExtractionSubsystem::LoadProfile()
{
    if (bLoaded) return bStorageHealthy && Profile != nullptr;
    bLoaded = true;
    bool Exists = false;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FString Slot = SlotPrefix + FString::Printf(TEXT("_%d"), Index);
        Exists |= UGameplayStatics::DoesSaveGameExist(Slot, 0);
        auto* C = Cast<UFPSExtractionSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
        if (C && C->Version == 1) UpgradeProfile(C);
        if (ValidateProfile(C) && (!Profile || C->Revision > Profile->Revision)) Profile = C;
    }
    if (Profile) return true;
    if (Exists) { bStorageHealthy = false; return Fail(TEXT("实验存档无法读取或内容不兼容，已保留原文件。")); }
    auto* C = NewObject<UFPSExtractionSave>(this);
    C->Version = 2;
    auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    C->Loadout.Primary = Catalog->CreateInstance(TEXT("Weapon.M4")); C->Loadout.Secondary = Catalog->CreateInstance(TEXT("Weapon.M9"));
    return Commit(C);
}
bool UFPSExtractionSubsystem::EnterWorld(bool bRaid)
{
    if (!IsAuthorized() || !LoadProfile()) return false;
    Backpack.Reset(); ExtractionRemaining = -1; Interaction.Empty();
    if (bRaid && Profile->bExpeditionActive) { Backpack = Profile->PreparedBackpack; Phase = EFPSExtractionPhase::Raid; Feedback = TEXT("搜索物资并返回撤离点；Tab 打开背包。" ); return true; }
    Phase = EFPSExtractionPhase::Base;
    if (Profile->bExpeditionActive)
    {
        auto* C = CopyProfile(); C->bExpeditionActive = false;
        if (!Commit(C)) return false;
        Feedback = TEXT("上次出征中断：未撤离物资已丢失，出征装备保留。" );
    }
    else Feedback = TEXT("准备装备，前往工业关隘回收物资。" );
    return !bRaid;
}
void UFPSExtractionSubsystem::Travel(bool bRaid)
{
    UGameplayStatics::OpenLevel(this, FName(LabMap), true,
        FString(TEXT("game=/Script/MCP_Test.FPSExtractionMode?Mode=Solo?Bots=0?LabPhase=")) + (bRaid ? TEXT("Raid") : TEXT("Base")));
}
bool UFPSExtractionSubsystem::StartExpedition()
{
    if (!IsBase()) return Fail(TEXT("只能从基地出征。"));
    auto* C = CopyProfile(); C->bExpeditionActive = true; ClearRuntime(C->Loadout);
    if (!Commit(C)) return false;
    LastHaul.Reset(); Backpack.Reset(); Phase = EFPSExtractionPhase::Raid; Travel(true); return true;
}
bool UFPSExtractionSubsystem::ReturnToBase()
{
    if (!IsAuthorized() || Phase != EFPSExtractionPhase::Result) return Fail(TEXT("请先完成撤离或结束本次出征。"));
    Phase = EFPSExtractionPhase::Base; Travel(false); return true;
}
bool UFPSExtractionSubsystem::Collect(FName Id, int32 Quantity)
{
    if (!IsAuthorized() || Phase != EFPSExtractionPhase::Raid || !Profile || !Profile->bExpeditionActive) return false;
    if (!TryAdd(Backpack, Id, Quantity, BagCapacity)) return Fail(TEXT("背包空间不足，物资仍在原处。"));
    Feedback = FString::Printf(TEXT("已回收 %s × %d"), *ItemName(Id).ToString(), Quantity); return true;
}
bool UFPSExtractionSubsystem::FinishExpedition(bool bSuccess)
{
    if (!IsAuthorized() || Phase != EFPSExtractionPhase::Raid || !Profile || !Profile->bExpeditionActive) return false;
    auto* C = CopyProfile();
    if (bSuccess)
        for (const auto& S : Backpack)
            if (!TryAdd(C->Warehouse, S.ItemId, S.Quantity, StoreCapacity)) return Fail(TEXT("仓库空间不足，撤离未结算；请丢弃部分搜获后重试。"));
    if (auto* Body = Cast<AFPSDemoCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
        C->Loadout = Body->GetWeapons()->GetLoadout();
    ClearRuntime(C->Loadout); C->bExpeditionActive = false;
    if (bSuccess) C->PreparedBackpack.Reset();
    if (!Commit(C)) return false;
    LastHaul = Backpack;
    if (!bSuccess)
        for (const auto& S : Profile->PreparedBackpack)
        {
            const int32 Retained = FMath::Min(S.Quantity, Count(LastHaul, S.ItemId));
            if (Retained > 0) TryRemove(LastHaul, S.ItemId, Retained);
        }
    Backpack.Reset(); Phase = EFPSExtractionPhase::Result; bLastSuccess = bSuccess; ExtractionRemaining = -1;
    Feedback = bSuccess ? TEXT("撤离成功，背包物资已入库。") : TEXT("本次搜获已丢失；出征枪械、配件保留，原整备物资返回基地背包。" );
    if (auto* GS = GetWorld()->GetGameState<AFPSDemoGameState>()) GS->bFinished = true;
    return true;
}
bool UFPSExtractionSubsystem::AbortExpedition() { return FinishExpedition(false); }
bool UFPSExtractionSubsystem::RestoreEquipment(AFPSDemoCharacter* Body)
{
    if (!IsAuthorized() || !Profile || Phase != EFPSExtractionPhase::Raid || !Body) return false;
    FString Error;
    if (!Body->GetWeapons()->RestoreExtractionLoadout(Profile->Loadout, Error)) return Fail(Error);
    return true;
}
bool UFPSExtractionSubsystem::Sell(FName Id, int32 Quantity)
{
    const auto* I = FindItem(Id);
    if (!IsBase() || !I || Quantity <= 0) return Fail(TEXT("当前不能出售。"));
    auto* C = CopyProfile(); const int64 Value = int64(Quantity) * I->SellPrice;
    if (!TryRemove(C->Warehouse, Id, Quantity) || int64(C->Credits) + Value > MAX_int32) return Fail(TEXT("库存数量不足或金额超出范围。"));
    C->Credits += int32(Value); if (!Commit(C)) return false; Feedback = TEXT("出售完成。" ); return true;
}
bool UFPSExtractionSubsystem::StartProduction(FName Id)
{
    const auto* R = FindRecipe(Id);
    if (!IsBase() || !R) return Fail(TEXT("当前无法生产。"));
    if (!Profile->ProductionRecipe.IsNone()) return Fail(TEXT("加工台已占用，请先领取产物。"));
    auto* C = CopyProfile();
    if (C->Credits < R->CreditCost) return Fail(TEXT("资金不足。"));
    for (const auto& I : R->Inputs) if (!TryRemove(C->Warehouse, I.ItemId, I.Quantity)) return Fail(TEXT("制作材料不足。"));
    C->Credits -= R->CreditCost; C->ProductionRecipe = Id; C->ProductionEnds = FDateTime::UtcNow() + FTimespan::FromSeconds(R->Duration);
    if (!Commit(C)) return false; Feedback = TEXT("生产已开始，材料已投入。" ); return true;
}
bool UFPSExtractionSubsystem::ClaimProduction()
{
    if (!IsBase()) return Fail(TEXT("请返回基地领取。"));
    const auto* R = FindRecipe(Profile->ProductionRecipe);
    if (!R || FDateTime::UtcNow() < Profile->ProductionEnds) return Fail(TEXT("没有已完成的产物。"));
    auto* C = CopyProfile();
    if (!TryAdd(C->Warehouse, R->Output.ItemId, R->Output.Quantity, StoreCapacity)) return Fail(TEXT("仓库已满，产物保留在加工台。"));
    C->ProductionRecipe = NAME_None; C->ProductionEnds = FDateTime();
    if (!Commit(C)) return false; Feedback = TEXT("产物已入库。" ); return true;
}
bool UFPSExtractionSubsystem::InstallAttachment(FName Id)
{
    const auto* I = FindItem(Id);
    if (!IsBase() || !I || I->AttachmentId.IsNone()) return Fail(TEXT("请选择仓库中的配件。"));
    auto* Catalog = GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>();
    const auto* Part = Catalog->FindAttachment(I->AttachmentId);
    if (!Part) return Fail(TEXT("配件定义未加载。"));
    auto* C = CopyProfile(); auto& Gun = C->Loadout.Primary;
    if (Gun.AttachmentIds.Contains(I->AttachmentId)) return Fail(TEXT("已安装该配件。"));
    if (!TryRemove(C->Warehouse, Id, 1)) return Fail(TEXT("仓库中没有该配件。"));
    for (int32 N = Gun.AttachmentIds.Num() - 1; N >= 0; --N)
    {
        const auto* Old = Catalog->FindAttachment(Gun.AttachmentIds[N]);
        if (Old && Old->Category == Part->Category)
        {
            const auto* OldItem = Items.FindByPredicate([Old](const auto& X) { return X.AttachmentId == Old->AttachmentId; });
            if (!OldItem || !TryAdd(C->Warehouse, OldItem->ItemId, 1, StoreCapacity)) return Fail(TEXT("无法退回旧配件，装配未改变。"));
            Gun.AttachmentIds.RemoveAt(N);
        }
    }
    Gun.AttachmentIds.Add(I->AttachmentId);
    FFPSResolvedWeaponStats Stats; TArray<FString> Errors;
    if (!Catalog->Resolve(Gun.WeaponId, Gun.AttachmentIds, Stats, Errors) || Gun.ConfigurationRevision == MAX_uint32) return Fail(TEXT("配件不兼容或配置版本无效。"));
    const int64 Total = int64(Gun.Magazine) + Gun.Reserve;
    // Base assembly redistributes existing rounds; it never manufactures ammunition.
    Gun.Magazine = int32(FMath::Min<int64>(Total, Stats.MagazineSize));
    if (Total - Gun.Magazine > MAX_int32) return Fail(TEXT("备弹超出范围。"));
    Gun.Reserve = int32(Total - Gun.Magazine); ++Gun.ConfigurationRevision;
    if (!Commit(C)) return false; Feedback = TEXT("配件已安装，下次出征使用新配置。" ); return true;
}
bool UFPSExtractionSubsystem::RemoveAttachment(FName Id)
{
    if (!IsBase() || !Profile->Loadout.Primary.AttachmentIds.Contains(Id)) return Fail(TEXT("未安装该配件。"));
    const auto* I = Items.FindByPredicate([Id](const auto& X) { return X.AttachmentId == Id; });
    auto* C = CopyProfile(); auto& Gun = C->Loadout.Primary;
    if (!I || !TryAdd(C->Warehouse, I->ItemId, 1, StoreCapacity)) return Fail(TEXT("仓库空间不足，拆卸未提交。"));
    Gun.AttachmentIds.Remove(Id); FFPSResolvedWeaponStats Stats; TArray<FString> Errors;
    if (!GetGameInstance()->GetSubsystem<UFPSWeaponCatalog>()->Resolve(Gun.WeaponId, Gun.AttachmentIds, Stats, Errors) || Gun.ConfigurationRevision == MAX_uint32) return Fail(TEXT("拆卸后组合无效。"));
    const int64 Total = int64(Gun.Magazine) + Gun.Reserve; Gun.Magazine = int32(FMath::Min<int64>(Total, Stats.MagazineSize));
    if (Total - Gun.Magazine > MAX_int32) return Fail(TEXT("备弹超出范围。"));
    Gun.Reserve = int32(Total - Gun.Magazine); ++Gun.ConfigurationRevision;
    if (!Commit(C)) return false; Feedback = TEXT("配件已退回仓库。" ); return true;
}
bool UFPSExtractionSubsystem::Resupply()
{
    if (!IsBase()) return Fail(TEXT("请返回基地补给。"));
    auto* C = CopyProfile(); if (C->Credits < 10) return Fail(TEXT("补给需要 10 资金。"));
    if (C->Loadout.Primary.Reserve >= 90 && C->Loadout.Secondary.Reserve >= 45) return Fail(TEXT("备弹充足，无需补给。"));
    C->Credits -= 10; C->Loadout.Primary.Reserve = FMath::Max(C->Loadout.Primary.Reserve, 90); C->Loadout.Secondary.Reserve = FMath::Max(C->Loadout.Secondary.Reserve, 45);
    if (!Commit(C)) return false; Feedback = TEXT("备弹已补给，枪内弹药保持不变。" ); return true;
}
bool UFPSExtractionSubsystem::DropItem(FName Id, int32 Quantity)
{
    if (!IsAuthorized() || Phase != EFPSExtractionPhase::Raid || Quantity <= 0 || Count(Backpack, Id) < Quantity) return Fail(TEXT("无法丢弃该物品。"));
    auto* PC = Cast<AFPSExtractionController>(UGameplayStatics::GetPlayerController(this, 0));
    auto* Mode = GetWorld()->GetAuthGameMode<AFPSExtractionMode>();
    if (!PC || !Mode || !Mode->DropCargo(PC, Id, Quantity)) return Fail(TEXT("当前无法放置掉落物，库存保持不变。"));
    TryRemove(Backpack, Id, Quantity); Feedback = TEXT("物资已放回地面。" ); return true;
}
void UFPSExtractionSubsystem::SetInteraction(const FString& Text, float Distance, float Remaining)
{ Interaction = Text; ExtractionDistance = Distance; ExtractionRemaining = Remaining; }
FFPSExtractionView UFPSExtractionSubsystem::GetView() const
{
    FFPSExtractionView V; V.Phase = Phase; V.Backpack = (Phase == EFPSExtractionPhase::Base && Profile) ? Profile->PreparedBackpack : Backpack; V.LastHaul = LastHaul; V.bLastSuccess = bLastSuccess;
    V.bStorageHealthy = bStorageHealthy && Profile != nullptr; V.Feedback = Feedback; V.Interaction = Interaction;
    V.ExtractionDistance = ExtractionDistance; V.ExtractionRemaining = ExtractionRemaining;
    V.BackpackSlots = BagCapacity; V.WarehouseSlots = StoreCapacity; V.BackpackUsed = SlotsUsed(V.Backpack);
    if (Profile)
    {
        V.Warehouse = Profile->Warehouse; V.Loadout = Profile->Loadout; V.Credits = Profile->Credits; V.WarehouseUsed = SlotsUsed(Profile->Warehouse);
        V.ProductionRecipe = Profile->ProductionRecipe;
        V.ProductionRemaining = Profile->ProductionRecipe.IsNone() ? 0.f : FMath::Max(0.f, float((Profile->ProductionEnds - FDateTime::UtcNow()).GetTotalSeconds()));
        V.bProductionReady = !Profile->ProductionRecipe.IsNone() && V.ProductionRemaining <= 0;
    }
    return V;
}
