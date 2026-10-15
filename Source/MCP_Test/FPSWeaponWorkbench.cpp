#include "FPSWeaponAuthoring.h"
#if WITH_EDITOR
#include "FPSAttachmentData.h"
#include "Editor.h"
#include "HAL/IConsoleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Misc/MessageDialog.h"
#include "UObject/GCObject.h"
#include "ToolMenus.h"
#include "Framework/Application/SlateApplication.h"
#include "Containers/Ticker.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "ImageUtils.h"
#include "ObjectTools.h"
#include "PackageTools.h"

namespace
{
const FName WorkbenchTab(TEXT("FPSWeaponWorkbench"));
FDelegateHandle StartupHandle;
struct FDefinitionRow { UObject* Asset = nullptr; };
struct FValidationRow { UObject* Asset = nullptr; FString Message; };

FName DefinitionId(UObject* Asset)
{
    if (const auto* Weapon = Cast<UFPSWeaponDefinition>(Asset)) return Weapon->WeaponId;
    if (const auto* Part = Cast<UFPSAttachmentDefinition>(Asset)) return Part->AttachmentId;
    return NAME_None;
}
FString SearchText(UObject* Asset)
{
    if (const auto* Weapon = Cast<UFPSWeaponDefinition>(Asset))
    {
        FString Text = Weapon->PlatformId.ToString() + TEXT(" ") + Weapon->WeaponId.ToString() + TEXT(" ") + Weapon->DisplayName.ToString();
        for (FName Mount : Weapon->MountIds) Text += TEXT(" ") + Mount.ToString();
        return Text;
    }
    if (const auto* Part = Cast<UFPSAttachmentDefinition>(Asset))
    {
        FString Text = Part->AttachmentId.ToString() + TEXT(" ") + Part->MountId.ToString() + TEXT(" ") + Part->DisplayName.ToString() + TEXT(" ") + UEnum::GetValueAsString(Part->Category);
        for (FName Platform : Part->Platforms) Text += TEXT(" ") + Platform.ToString();
        return Text + (Part->bUniversal ? TEXT(" Universal 通用") : TEXT(""));
    }
    return FString();
}

FString ColumnLabel(FName Field)
{
    static const TMap<FName, FString> Labels = {
        {TEXT("MagazineSize"), TEXT("容量（发）")}, {TEXT("Damage"), TEXT("基础伤害")},
        {TEXT("FireInterval"), TEXT("射击间隔（秒）")}, {TEXT("Range"), TEXT("射程（厘米）")},
        {TEXT("ReloadDuration"), TEXT("换弹时长（秒）")}, {TEXT("AimTransitionTime"), TEXT("ADS 时长（秒）")},
        {TEXT("AimFOV"), TEXT("瞄准 FOV（度）")}, {TEXT("CapacityDelta"), TEXT("容量增量（发）")},
        {TEXT("ReloadTimeMultiplier"), TEXT("换弹时长倍率")}, {TEXT("VerticalRecoilMultiplier"), TEXT("垂直后坐力倍率")},
        {TEXT("HorizontalRecoilMultiplier"), TEXT("水平后坐力倍率")}, {TEXT("AimTimeMultiplier"), TEXT("ADS 时长倍率")}, {TEXT("RangeMultiplier"), TEXT("最大射程倍率")}
    };
    const auto* Label = Labels.Find(Field); return Label ? *Label : Field.ToString();
}
FText ColumnHelp(FName Field)
{
    FString Help = Field.ToString() + TEXT("\n") + ColumnLabel(Field);
    if (Field == TEXT("CapacityDelta")) Help += TEXT("；整数加值，最终容量必须大于 0，仅弹匣可修改。");
    else if (Field == TEXT("MagazineSize")) Help += TEXT("；正整数，基础后坐力曲线需覆盖此容量。");
    else if (Field == TEXT("AimFOV")) Help += TEXT("；1–179 度，瞄具需启用 Override Aim。");
    else if (Field == TEXT("AimTransitionTime")) Help += TEXT("；有限且 ≥ 0，0 表示立即切换。");
    else if (Field == TEXT("VerticalRecoilMultiplier") || Field == TEXT("HorizontalRecoilMultiplier")) Help += TEXT("；有限且 ≥ 0，枪口、后握把、枪托和枪管可修改。");
    else Help += TEXT("；有限且 > 0，详情页提供字段及资源配置。");
    return FText::FromString(Help);
}

class SFPSWeaponWorkbench : public SCompoundWidget, public FGCObject
{
public:
    SLATE_BEGIN_ARGS(SFPSWeaponWorkbench) {} SLATE_END_ARGS()
    void Construct(const FArguments&)
    {
        TArray<FString> Errors; FPSWeaponAuthoring::Load(Assets, Errors);
        Status = Errors.IsEmpty() ? TEXT("直接编辑定义资产。* 表示未保存；双击行打开资产编辑器。") : FString::Join(Errors, TEXT("\n"));
        FDetailsViewArgs Args; Args.bAllowSearch = true; Args.bHideSelectionTip = true;
        Details = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).CreateDetailView(Args);
        Details->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateLambda([](const FPropertyAndParent& Info)
        {
            return Info.Property.GetFName() == TEXT("WeaponId") || Info.Property.GetFName() == TEXT("AttachmentId") || Info.Property.GetFName() == TEXT("SavedResourceVersions");
        }));
        for (UObject* Asset : Assets)
            if (auto* Weapon = Cast<UFPSWeaponDefinition>(Asset)) WeaponOptions.Add(MakeShared<FString>(Weapon->WeaponId.ToString()));
        ChildSlot
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("武器表"), [this]() { SetPage(0); })]
                + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("配件表"), [this]() { SetPage(1); })]
                + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("装配预览"), [this]() { Switcher->SetActiveWidgetIndex(1); })]
                + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("全项目校验"), [this]() { ValidateAll(); Switcher->SetActiveWidgetIndex(2); })]
                + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("撤销"), [this]() { if (GEditor) GEditor->UndoTransaction(); RefreshRows(); UpdatePreview(); })]
                + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("重做"), [this]() { if (GEditor) GEditor->RedoTransaction(); RefreshRows(); UpdatePreview(); })]
            ]
            + SVerticalBox::Slot().FillHeight(1)
            [
                SAssignNew(Switcher, SWidgetSwitcher)
                + SWidgetSwitcher::Slot()[BuildTable()]
                + SWidgetSwitcher::Slot()[BuildPreview()]
                + SWidgetSwitcher::Slot()
                [
                    SAssignNew(Issues, SListView<TSharedPtr<FValidationRow>>).ListItemsSource(&ValidationRows)
                    .OnGenerateRow_Lambda([](TSharedPtr<FValidationRow> Row, const TSharedRef<STableViewBase>& Owner)
                    {
                        return SNew(STableRow<TSharedPtr<FValidationRow>>, Owner)
                            [SNew(SButton).Text(FText::FromString(Row->Message)).OnClicked_Lambda([Row]()
                            {
                                if (GEditor && Row->Asset) GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Row->Asset);
                                return FReply::Handled();
                            })];
                    })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(8)
            [SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return FText::FromString(Status); })]
        ];
        SetPage(0);
        if (!WeaponOptions.IsEmpty())
        {
            auto* Initial = WeaponOptions.FindByPredicate([](const auto& Id) { return *Id == TEXT("Weapon.M4"); });
            WeaponChoice->SetSelectedItem(Initial ? *Initial : WeaponOptions[0]);
        }
    }
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override { Collector.AddReferencedObjects(Assets); Collector.AddReferencedObject(SmokeFixture); }
    virtual FString GetReferencerName() const override { return TEXT("SFPSWeaponWorkbench"); }
    void StartSmoke()
    {
        FParse::Value(FCommandLine::Get(), TEXT("FPSWorkbenchOutput="), SmokeOutput);
        if (SmokeOutput.IsEmpty()) SmokeOutput = FPaths::ProjectSavedDir() / TEXT("FPSAttachmentTests/A2-Editor");
        IFileManager::Get().MakeDirectory(*SmokeOutput, true);
        const TWeakPtr<SFPSWeaponWorkbench> Weak = SharedThis(this);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float)
        {
            const auto Widget = Weak.Pin();
            return Widget && Widget->SmokeStep();
        }), 1.f);
    }
private:
    TArray<TObjectPtr<UObject>> Assets;
    TArray<TSharedPtr<FDefinitionRow>> Rows;
    TArray<TSharedPtr<FValidationRow>> ValidationRows;
    TArray<FName> Fields;
    TSharedPtr<SListView<TSharedPtr<FDefinitionRow>>> List;
    TSharedPtr<SListView<TSharedPtr<FValidationRow>>> Issues;
    TSharedPtr<IDetailsView> Details;
    TSharedPtr<SWidgetSwitcher> Switcher;
    TSharedPtr<SHorizontalBox> Header;
    TSharedPtr<SEditableTextBox> BatchField, BatchValue;
    TArray<TSharedPtr<FString>> WeaponOptions, PartOptions[FPSAttachments::CategoryCount];
    TSharedPtr<STextComboBox> WeaponChoice, PartChoices[FPSAttachments::CategoryCount];
    UFPSWeaponDefinition* PreviewWeapon = nullptr;
    FString Filter, Status, PreviewText;
    int32 Page = 0;
    TMap<UObject*, TWeakPtr<SEditableTextBox>> ReloadEditors;
    TObjectPtr<UFPSWeaponDefinition> SmokeFixture = nullptr;
    FString SmokeOutput, SmokeAssetPath;
    float SmokeOriginalReload = 0;
    int32 SmokeStage = 0, SmokeRetries = 0;
    bool bSmokePassed = true, bRecoveryDone = false, bRecoveryDetached = false;

    void SmokeCheck(bool Passed, const TCHAR* Label)
    {
        bSmokePassed &= Passed;
        UE_LOG(LogTemp, Display, TEXT("FPS_WORKBENCH_TEST %s %s"), Passed ? TEXT("PASS") : TEXT("FAIL"), Label);
    }
    bool Capture(const TCHAR* Name)
    {
        TArray<FColor> Pixels; FIntVector Size;
        if (!FSlateApplication::Get().TakeScreenshot(SharedThis(this), Pixels, Size)) return false;
        TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
        return FFileHelper::SaveArrayToFile(PNG, *(SmokeOutput / Name));
    }
    bool SmokeStep()
    {
        FString Failure;
        // Recovery is limited to an explicitly named template copy from an interrupted test.
        FString RecoveryPath;
        if (SmokeStage == 0 && FParse::Value(FCommandLine::Get(), TEXT("FPSWorkbenchCleanup="), RecoveryPath) && !bRecoveryDone)
        {
            if (auto* Fixture = LoadObject<UFPSWeaponDefinition>(nullptr, *RecoveryPath))
            {
                if (!Fixture->WeaponId.ToString().StartsWith(TEXT("Weapon.Copy."))) { SmokeCheck(false, TEXT("recovery refuses non-test definition")); SmokeStage = 9; return true; }
                Assets.Remove(Fixture); Details->SetObject(nullptr); RefreshRows();
                if (!bRecoveryDetached) { bRecoveryDetached = true; return true; }
                SmokeCheck(ObjectTools::DeleteObjectsUnchecked({Fixture}) == 1, TEXT("previous interrupted fixture cleaned through editor"));
            }
            bRecoveryDone = true;
        }
        if (SmokeStage == 0)
        {
            const auto* Source = Assets.FindByPredicate([](UObject* Asset) { return DefinitionId(Asset) == TEXT("Weapon.M4"); });
            SmokeFixture = Source ? Cast<UFPSWeaponDefinition>(FPSWeaponAuthoring::CopyTemplate(*Source, Failure)) : nullptr;
            SmokeCheck(SmokeFixture != nullptr, TEXT("template creates new definition"));
            if (!SmokeFixture) { SmokeStage = 9; return true; }
            SmokeAssetPath = SmokeFixture->GetPathName(); SmokeOriginalReload = SmokeFixture->ReloadDuration;
            SmokeCheck(SmokeFixture->WeaponId != TEXT("Weapon.M4") && SmokeFixture->FirstPerson.Mesh == CastChecked<UFPSWeaponDefinition>(*Source)->FirstPerson.Mesh, TEXT("copy has new stable ID and explicit shared resources"));
            Assets.Add(SmokeFixture); Filter.Empty(); SetPage(0);
            for (const auto& Row : Rows) if (Row->Asset == SmokeFixture) { List->SetSelection(Row); List->RequestScrollIntoView(Row); }
        }
        else if (SmokeStage == 1)
        {
            const auto Edit = ReloadEditors.FindRef(SmokeFixture).Pin();
            if (!Edit && ++SmokeRetries < 10) return true;
            SmokeCheck(Edit.IsValid(), TEXT("editable table cell generated"));
            if (Edit)
            {
                FSlateApplication::Get().SetKeyboardFocus(Edit, EFocusCause::SetDirectly);
                Edit->SelectAllText();
                for (TCHAR Character : FString(TEXT("1.75"))) FSlateApplication::Get().ProcessKeyCharEvent(FCharacterEvent(Character, FModifierKeysState(), 0, false));
                FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0));
            }
            SmokeCheck(FMath::IsNearlyEqual(SmokeFixture->ReloadDuration, 1.75f) && SmokeFixture->GetOutermost()->IsDirty(), TEXT("Slate input commits numeric edit and marks dirty"));
        }
        else if (SmokeStage == 2)
        {
            SmokeCheck(GEditor->UndoTransaction() && SmokeFixture->ReloadDuration == SmokeOriginalReload, TEXT("numeric edit undo restores source value"));
            SmokeCheck(GEditor->RedoTransaction() && FMath::IsNearlyEqual(SmokeFixture->ReloadDuration, 1.75f), TEXT("numeric edit redo restores new value"));
            SmokeCheck(!FPSWeaponAuthoring::SetNumber({SmokeFixture}, TEXT("ReloadDuration"), TEXT("1.7junk"), Failure) && FMath::IsNearlyEqual(SmokeFixture->ReloadDuration, 1.75f), TEXT("malformed input rejected without mutation"));
            SmokeCheck(!FPSWeaponAuthoring::SetNumber({SmokeFixture}, TEXT("MagazineSize"), TEXT("1.5"), Failure), TEXT("integer column rejects fractional input"));
            UObject* Part = nullptr; for (UObject* Asset : Assets) if (Cast<UFPSAttachmentDefinition>(Asset)) { Part = Asset; break; }
            SmokeCheck(!FPSWeaponAuthoring::SetNumber({SmokeFixture, Part}, TEXT("ReloadDuration"), TEXT("1.25"), Failure) && FMath::IsNearlyEqual(SmokeFixture->ReloadDuration, 1.75f), TEXT("invalid later batch item leaves first item unchanged"));
            const float OriginalDamage = SmokeFixture->Damage;
            SmokeFixture->Damage = -1;
            ValidateAll();
            SmokeCheck(ValidationRows.ContainsByPredicate([this](const auto& Row) { return Row->Asset == SmokeFixture && Row->Message.Contains(TEXT("Damage")); }), TEXT("validation error locates offending definition"));
            SmokeFixture->Damage = OriginalDamage;
            SmokeCheck(FPSWeaponAuthoring::Save({SmokeFixture}, Failure) && !SmokeFixture->GetOutermost()->IsDirty(), TEXT("explicit save writes package and clears dirty"));
            FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
            Details->SetObject(nullptr); Assets.Remove(SmokeFixture); ReloadEditors.Empty(); RefreshRows();
            GEditor->ResetTransaction(FText::FromString(TEXT("清理工作台自测事务")));
        }
        else if (SmokeStage == 3)
        {
            UPackage* Package = SmokeFixture->GetOutermost(); SmokeFixture = nullptr;
            FText Error;
            SmokeCheck(UPackageTools::UnloadPackages({Package}, Error, false), TEXT("saved fixture package unloaded"));
            SmokeFixture = LoadObject<UFPSWeaponDefinition>(nullptr, *SmokeAssetPath);
            SmokeCheck(SmokeFixture && FMath::IsNearlyEqual(SmokeFixture->ReloadDuration, 1.75f), TEXT("fresh asset load retains edited value"));
            if (SmokeFixture)
            {
                UObject* ToDelete = SmokeFixture;
                SmokeFixture = nullptr;
                SmokeCheck(ObjectTools::DeleteObjectsUnchecked({ToDelete}) == 1, TEXT("temporary fixture deleted through editor"));
            }
            const FString PackageName = FPackageName::ObjectPathToPackageName(SmokeAssetPath);
            SmokeCheck(!FPackageName::DoesPackageExist(PackageName), TEXT("temporary saved package removed"));
            SetPage(0);
        }
        else if (SmokeStage == 4) { SmokeCheck(Capture(TEXT("Weapons.png")), TEXT("weapon table screenshot")); SetPage(1); }
        else if (SmokeStage == 5)
        {
            SmokeCheck(Capture(TEXT("Attachments.png")), TEXT("attachment table screenshot"));
            Switcher->SetActiveWidgetIndex(1);
            const TCHAR* SelectedIds[] = { TEXT("Attachment.M4_OpticFixed"), TEXT("Attachment.M4_MuzzleBrake"), TEXT("Attachment.M4_MagazineExtended") };
            for (int32 Index = 0; Index < UE_ARRAY_COUNT(SelectedIds); ++Index)
                if (const auto* Option = PartOptions[Index].FindByPredicate([&](const auto& Item) { return *Item == SelectedIds[Index]; })) PartChoices[Index]->SetSelectedItem(*Option);
            UpdatePreview();
            SmokeCheck(PreviewText.Contains(TEXT("30 → 45")) && PreviewText.Contains(TEXT("48.00")), TEXT("preview resolves saved three-part combination"));
        }
        else if (SmokeStage == 6) { SmokeCheck(Capture(TEXT("Preview.png")), TEXT("assembly preview screenshot")); ValidateAll(); Switcher->SetActiveWidgetIndex(2); }
        else if (SmokeStage == 7) { SmokeCheck(ValidationRows.IsEmpty(), TEXT("validation page reports clean saved catalog")); SmokeCheck(Capture(TEXT("Validation.png")), TEXT("validation page screenshot")); }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("FPS_WORKBENCH_TEST %s COMPLETE"), bSmokePassed ? TEXT("PASS") : TEXT("FAIL"));
            if (FParse::Param(FCommandLine::Get(), TEXT("FPSWorkbenchExit"))) FPlatformMisc::RequestExit(false);
            return false;
        }
        ++SmokeStage; return true;
    }

    TSharedRef<SWidget> Button(const TCHAR* Label, TFunction<void()> Action)
    {
        return SNew(SButton).Text(FText::FromString(Label)).OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); });
    }
    TArray<UObject*> Selected() const
    {
        TArray<UObject*> Result;
        if (List) for (const auto& Row : List->GetSelectedItems()) Result.Add(Row->Asset);
        return Result;
    }
    TSharedRef<SWidget> BuildTable()
    {
        return SNew(SSplitter)
            + SSplitter::Slot().Value(.72f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(4)
                [SNew(SEditableTextBox).HintText(FText::FromString(TEXT("筛选：平台、接口、类别或型号；可用空格组合关键词")))
                    .OnTextChanged_Lambda([this](const FText& Text) { Filter = Text.ToString(); RefreshRows(); })]
                + SVerticalBox::Slot().AutoHeight().Padding(4)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("保存选中资产"), [this]() { SaveSelected(); })]
                    + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("从选中模板创建"), [this]() { CopySelected(); })]
                    + SHorizontalBox::Slot().FillWidth(1)[SAssignNew(BatchField, SEditableTextBox).HintText(FText::FromString(TEXT("批量字段（列名）")))]
                    + SHorizontalBox::Slot().FillWidth(1)[SAssignNew(BatchValue, SEditableTextBox).HintText(FText::FromString(TEXT("新值")))]
                    + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("预览批量修改"), [this]() { Batch(); })]
                ]
                + SVerticalBox::Slot().FillHeight(1)
                [
                    SNew(SScrollBox).Orientation(Orient_Horizontal)
                    + SScrollBox::Slot()
                    [
                        SNew(SBox).WidthOverride(1230)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight()[SAssignNew(Header, SHorizontalBox)]
                            + SVerticalBox::Slot().FillHeight(1)
                            [
                                SAssignNew(List, SListView<TSharedPtr<FDefinitionRow>>).ListItemsSource(&Rows).SelectionMode(ESelectionMode::Multi)
                                .OnGenerateRow_Lambda([this](TSharedPtr<FDefinitionRow> Row, const TSharedRef<STableViewBase>& Owner)
                                {
                                    auto Cells = SNew(SHorizontalBox);
                                    Cells->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(270)
                                        [SNew(STextBlock).Text_Lambda([Row]() { return FText::FromString((Row->Asset->GetOutermost()->IsDirty() ? TEXT("* ") : TEXT("")) + DefinitionId(Row->Asset).ToString()); }).ToolTipText(FText::FromString(Row->Asset->GetPathName()))]];
                                    for (FName Field : Fields)
                                    {
                                        TSharedPtr<SEditableTextBox> Cell;
                                        Cells->AddSlot().AutoWidth().Padding(2)[SNew(SBox).WidthOverride(112)
                                            [SAssignNew(Cell, SEditableTextBox).ToolTipText(ColumnHelp(Field)).Text_Lambda([Row, Field]() { return FText::FromString(FPSWeaponAuthoring::Number(Row->Asset, Field)); })
                                                .OnTextCommitted_Lambda([this, Row, Field](const FText& Text, ETextCommit::Type Commit)
                                                {
                                                    if (Commit == ETextCommit::OnCleared || Text.ToString() == FPSWeaponAuthoring::Number(Row->Asset, Field)) return;
                                                    FString Failure;
                                                    Status = FPSWeaponAuthoring::SetNumber({Row->Asset}, Field, Text.ToString(), Failure) ? TEXT("已修改，尚未保存。") : Failure;
                                                    UpdatePreview();
                                                })]];
                                        if (Field == TEXT("ReloadDuration")) ReloadEditors.Add(Row->Asset, Cell);
                                    }
                                    return SNew(STableRow<TSharedPtr<FDefinitionRow>>, Owner)[Cells];
                                })
                                .OnSelectionChanged_Lambda([this](TSharedPtr<FDefinitionRow> Row, ESelectInfo::Type) { Details->SetObject(Row ? Row->Asset : nullptr); })
                                .OnMouseButtonDoubleClick_Lambda([](TSharedPtr<FDefinitionRow> Row) { if (GEditor) GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Row->Asset); })
                            ]
                        ]
                    ]
                ]
            ]
            + SSplitter::Slot().Value(.28f)[Details.ToSharedRef()];
    }
    void SetPage(int32 NewPage)
    {
        Page = NewPage;
        Fields = Page == 0 ? TArray<FName>{TEXT("MagazineSize"), TEXT("Damage"), TEXT("FireInterval"), TEXT("Range"), TEXT("ReloadDuration"), TEXT("AimTransitionTime"), TEXT("AimFOV")} :
            TArray<FName>{TEXT("CapacityDelta"), TEXT("ReloadTimeMultiplier"), TEXT("VerticalRecoilMultiplier"), TEXT("HorizontalRecoilMultiplier"), TEXT("AimTimeMultiplier"), TEXT("AimFOV"), TEXT("RangeMultiplier")};
        Header->ClearChildren();
        Header->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(270)[SNew(STextBlock).Text(FText::FromString(TEXT("稳定 ID / 未保存 *")))]];
        for (FName Field : Fields)
        {
            Header->AddSlot().AutoWidth().Padding(2)[SNew(SBox).WidthOverride(112)[SNew(STextBlock).AutoWrapText(true).ToolTipText(ColumnHelp(Field)).Text(FText::FromString(ColumnLabel(Field)))]];
        }
        Switcher->SetActiveWidgetIndex(0); RefreshRows();
    }
    void RefreshRows()
    {
        Rows.Empty(); TArray<FString> Tokens; Filter.ParseIntoArrayWS(Tokens);
        for (UObject* Asset : Assets)
        {
            if ((Page == 0) != (Cast<UFPSWeaponDefinition>(Asset) != nullptr)) continue;
            const FString Search = SearchText(Asset);
            if (Tokens.ContainsByPredicate([&](const FString& Token) { return !Search.Contains(Token); })) continue;
            auto Row = MakeShared<FDefinitionRow>(); Row->Asset = Asset; Rows.Add(Row);
        }
        if (List) List->RebuildList();
    }
    void SaveSelected()
    {
        FString Failure;
        Status = FPSWeaponAuthoring::Save(Selected(), Failure) ? TEXT("选中资产已保存。生成清单需从资产重新导出。") : Failure;
    }
    void CopySelected()
    {
        auto Selection = Selected();
        if (Selection.Num() != 1) { Status = TEXT("请选中一个模板。副本使用新稳定 ID，并共享模板的资源引用。"); return; }
        FString Failure;
        if (auto* Copy = FPSWeaponAuthoring::CopyTemplate(Selection[0], Failure))
        {
            Assets.Add(Copy); RefreshRows(); Details->SetObject(Copy);
            for (const auto& Row : Rows) if (Row->Asset == Copy) List->SetSelection(Row);
            if (auto* Weapon = Cast<UFPSWeaponDefinition>(Copy)) { WeaponOptions.Add(MakeShared<FString>(Weapon->WeaponId.ToString())); WeaponChoice->RefreshOptions(); }
            Status = TEXT("已创建未保存副本：") + Copy->GetPathName() + TEXT("。资源引用共享；可在详情中调整，随后保存。");
        }
        else Status = Failure;
    }
    void Batch()
    {
        const auto Selection = Selected();
        if (Selection.IsEmpty()) { Status = TEXT("请先选择受影响资产（Ctrl／Shift 多选）。"); return; }
        FString Message = BatchField->GetText().ToString() + TEXT(" = ") + BatchValue->GetText().ToString() + TEXT("\n\n将修改以下资产，尚不保存：\n");
        for (auto* Asset : Selection) Message += Asset->GetPathName() + TEXT("\n");
        if (FMessageDialog::Open(EAppMsgType::OkCancel, FText::FromString(Message)) != EAppReturnType::Ok) return;
        FString Failure;
        Status = FPSWeaponAuthoring::SetNumber(Selection, FName(*BatchField->GetText().ToString()), BatchValue->GetText().ToString(), Failure) ? TEXT("批量修改已应用，可整体撤销；尚未保存。") : Failure;
        UpdatePreview();
    }
    TSharedRef<SWidget> BuildPreview()
    {
        auto Box = SNew(SVerticalBox);
        Box->AddSlot().AutoHeight().Padding(5)[SNew(STextBlock).Text(FText::FromString(TEXT("武器与配件组合只作预览，不改变游戏或保存的默认装配。")))];
        Box->AddSlot().AutoHeight().Padding(5)[SAssignNew(WeaponChoice, STextComboBox).OptionsSource(&WeaponOptions)
            .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Id, ESelectInfo::Type)
            {
                PreviewWeapon = nullptr;
                if (Id) for (UObject* Asset : Assets) if (auto* Weapon = Cast<UFPSWeaponDefinition>(Asset)) if (Weapon->WeaponId.ToString() == *Id) PreviewWeapon = Weapon;
                for (int32 Index = 0; Index < FPSAttachments::CategoryCount; ++Index)
                {
                    PartOptions[Index].Empty(); PartOptions[Index].Add(MakeShared<FString>(TEXT("(原装 / 无附加配件)")));
                    if (PreviewWeapon) for (const auto& Ref : PreviewWeapon->AvailableAttachments)
                        if (const auto* Part = Ref.LoadSynchronous()) if (int32(Part->Category) == Index) PartOptions[Index].Add(MakeShared<FString>(Part->AttachmentId.ToString()));
                    PartChoices[Index]->RefreshOptions(); PartChoices[Index]->SetSelectedItem(PartOptions[Index][0]);
                }
                UpdatePreview();
            })];

        for (int32 Index = 0; Index < FPSAttachments::CategoryCount; ++Index)
        {
            Box->AddSlot().AutoHeight().Padding(5)[SNew(STextBlock).Text(FText::FromString(FPSAttachments::Label(EFPSAttachmentCategory(Index))))];
            Box->AddSlot().AutoHeight().Padding(5)[SAssignNew(PartChoices[Index], STextComboBox).OptionsSource(&PartOptions[Index])
                .OnSelectionChanged_Lambda([this](TSharedPtr<FString>, ESelectInfo::Type) { UpdatePreview(); })];
        }
        Box->AddSlot().AutoHeight().Padding(5)[Button(TEXT("重新计算"), [this]() { UpdatePreview(); })];
        Box->AddSlot().FillHeight(1).Padding(5)[SNew(SMultiLineEditableTextBox).IsReadOnly(true).Text_Lambda([this]() { return FText::FromString(PreviewText); })];
        return Box;
    }
    void UpdatePreview()
    {
        if (!PreviewWeapon) { PreviewText = TEXT("请选择武器。"); return; }
        TArray<const UFPSAttachmentDefinition*> Parts;
        for (int32 Index = 0; Index < FPSAttachments::CategoryCount; ++Index)
            if (PartChoices[Index]) if (const auto Choice = PartChoices[Index]->GetSelectedItem())
                if (Choice->StartsWith(TEXT("Attachment.")))
                {
                    const auto* Found = PreviewWeapon->AvailableAttachments.FindByPredicate([&](const auto& Ref) { return Ref.Get() && Ref->AttachmentId.ToString() == *Choice; });
                    Parts.Add(Found ? Found->Get() : nullptr);
                }
        TArray<FString> Errors; FPSWeaponAuthoring::Validate(PreviewWeapon, Errors);
        FFPSResolvedWeaponStats Stats;
        if (!FFPSAttachmentResolver::Resolve(*PreviewWeapon, Parts, Stats, Errors) || !Errors.IsEmpty()) { PreviewText = FString::Join(Errors, TEXT("\n")); return; }
        PreviewText = FString::Printf(TEXT("兼容；基础值 → 装配值\n\n容量：%d → %d\n换弹：%.3f → %.3f s\nADS 过渡：%.3f → %.3f s\n瞄准 FOV：%.2f → %.2f deg\n垂直后坐力倍率：1 → %.3f\n水平后坐力倍率：1 → %.3f\n瞄准位置：%s\n"),
            PreviewWeapon->MagazineSize, Stats.MagazineSize, PreviewWeapon->ReloadDuration, Stats.ReloadDuration, PreviewWeapon->AimTransitionTime, Stats.AimTransitionTime,
            PreviewWeapon->AimFOV, Stats.AimFOV, Stats.VerticalRecoilMultiplier, Stats.HorizontalRecoilMultiplier, *Stats.AimLocation.ToString());
        PreviewText += FString::Printf(TEXT("最大射程：%.2f → %.2f m\n"), PreviewWeapon->Range/100.f, Stats.Range/100.f);
    }
    void ValidateAll()
    {
        ValidationRows.Empty(); TArray<FString> Errors;
        TArray<UFPSWeaponDefinition*> Weapons; for (UObject* Asset : Assets) if (auto* Weapon = Cast<UFPSWeaponDefinition>(Asset)) Weapons.Add(Weapon);
        UFPSWeaponCatalog::ValidateCatalog(Weapons, Errors);
        for (const auto& Error : Errors)
        {
            auto Row = MakeShared<FValidationRow>(); Row->Message = Error;
            for (UObject* Asset : Assets) if (Error.Contains(Asset->GetPathName())) { Row->Asset = Asset; break; }
            ValidationRows.Add(Row);
        }
        Issues->RequestListRefresh();
        Status = FString::Printf(TEXT("全项目校验：%d 项错误。点击错误可打开对应资产；新副本先保存以注册身份。"), Errors.Num());
    }
};
}

namespace FPSWeaponAuthoring
{
void Open() { FGlobalTabmanager::Get()->TryInvokeTab(WorkbenchTab); }
void Register()
{
    if (!GIsEditor || IsRunningCommandlet()) return;
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(WorkbenchTab, FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
    {
        return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SFPSWeaponWorkbench)];
    })).SetDisplayName(FText::FromString(TEXT("枪械配件工作台")));
    StartupHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
    {
        FToolMenuOwnerScoped Owner(TEXT("FPSWeaponWorkbench"));
        auto* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
        Menu->FindOrAddSection(TEXT("FPSAuthoring")).AddMenuEntry(TEXT("FPSWeaponWorkbench"), FText::FromString(TEXT("枪械配件工作台")),
            FText::FromString(TEXT("表格编辑定义资产、预览装配与检查内容")), FSlateIcon(), FUIAction(FExecuteAction::CreateStatic(&Open)));
    }));
}
void Unregister()
{
    if (!GIsEditor || IsRunningCommandlet()) return;
    UToolMenus::UnRegisterStartupCallback(StartupHandle);
    if (UToolMenus::IsToolMenuUIEnabled()) UToolMenus::UnregisterOwner(TEXT("FPSWeaponWorkbench"));
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WorkbenchTab);
}
static FAutoConsoleCommand OpenCommand(TEXT("FPS.OpenWeaponWorkbench"), TEXT("Open the weapon/attachment authoring workbench"), FConsoleCommandDelegate::CreateStatic(&Open));
static FAutoConsoleCommand SmokeCommand(TEXT("FPS.WorkbenchSmoke"), TEXT("Test editing and capture the workbench in an actual editor"), FConsoleCommandDelegate::CreateLambda([]()
{
    if (!GIsEditor || IsRunningCommandlet()) return;
    if (const auto Tab = FGlobalTabmanager::Get()->TryInvokeTab(WorkbenchTab)) StaticCastSharedRef<SFPSWeaponWorkbench>(Tab->GetContent())->StartSmoke();
}));
}
#endif
