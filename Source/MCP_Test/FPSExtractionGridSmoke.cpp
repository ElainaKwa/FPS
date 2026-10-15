#include "FPSExtractionSmoke.h"
#include "FPSExtractionSubsystem.h"

bool FFPSExtractionSmoke::GridChecks(UFPSExtractionSubsystem* Loop)
{
#if !UE_BUILD_SHIPPING
    TArray<FFPSExtractionStack> Grid;
    if(!Loop->TryAdd(Grid,TEXT("Scrap"),25,20) || Grid.Num()!=2 || Loop->Count(Grid,TEXT("Scrap"))!=25) return false;
    const auto First=Grid[0].StackId;
    if(!First.IsValid() || First==Grid[1].StackId || !Loop->ValidateGrid(Grid,20))return false;
    auto Invalid=Grid; Invalid[1].X=Invalid[0].X; Invalid[1].Y=Invalid[0].Y;
    if(Loop->ValidateGrid(Invalid,20))return false;
    Invalid=Grid; Invalid[1].StackId=First;
    if(Loop->ValidateGrid(Invalid,20))return false;
    if(Loop->CanPlace({},TEXT("Magazine"),4,3,false,20) || !Loop->CanPlace({},TEXT("Magazine"),3,3,true,20))return false;
    if(Loop->CanPlace({},TEXT("Scrap"),MAX_int32,0,false,20) || Loop->CanPlace({},TEXT("Scrap"),-1,0,false,20))return false;
    if(Loop->TryRemove(Grid,TEXT("Scrap"),26) || Loop->Count(Grid,TEXT("Scrap"))!=25)return false;
    if(!Loop->TryRemove(Grid,TEXT("Scrap"),21) || Loop->Count(Grid,TEXT("Scrap"))!=4 || Grid.Num()!=1)return false;
    auto* Old=Loop->CopyProfile(); Old->Version=1; Old->Warehouse={{TEXT("Scrap"),41},{TEXT("Magazine"),1}};
    if(!Loop->UpgradeProfile(Old) || !Loop->ValidateProfile(Old) || Old->Warehouse.Num()!=4 || Loop->Count(Old->Warehouse,TEXT("Scrap"))!=41)return false;
    Old->PreparedBackpack.Add(Old->Warehouse[0]);
    if(Loop->ValidateProfile(Old))return false; // One physical ID cannot exist in two persistent containers.
    auto* Original=Loop->Profile.Get();
    auto* Seed=Loop->CopyProfile();
    if(!Loop->TryAdd(Seed->Warehouse,TEXT("Wire"),25,64) || !Loop->Commit(Seed))return false;
    const auto Source=Loop->Profile->Warehouse[0].StackId, Target=Loop->Profile->Warehouse[1].StackId;
    if(!Loop->MoveStack(EFPSExtractionContainer::Warehouse,Source,EFPSExtractionContainer::Backpack,0,0,false))return false;
    // Partial merge fills the target and retains excess at the source.
    if(!Loop->MoveStack(EFPSExtractionContainer::Backpack,Source,EFPSExtractionContainer::Warehouse,1,0,false) ||
        Loop->Count(Loop->Profile->PreparedBackpack,TEXT("Wire"))!=5 || Loop->Count(Loop->Profile->Warehouse,TEXT("Wire"))!=20)return false;
    if(!Loop->SellStack(Target) || Loop->Count(Loop->Profile->Warehouse,TEXT("Wire"))!=0)return false;
    auto* Restore=DuplicateObject<UFPSExtractionSave>(Original,Loop); Restore->Revision=Loop->Profile->Revision;
    return Loop->Commit(Restore);
#else
    return true;
#endif
}
