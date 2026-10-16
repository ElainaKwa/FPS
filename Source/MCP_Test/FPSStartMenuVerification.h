#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "FPSStartMenuVerification.generated.h"

/** Cross-map UI routing check, enabled only by the isolated start menu test. */
UCLASS()
class UFPSStartMenuVerification : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(FPSStartMenuVerification,STATGROUP_Tickables);}
    virtual void Tick(float DeltaSeconds) override;
private:
    int32 Stage=0;
    double LastStep=0;
    double ExitRequestedAt=0;
    bool bFailed=false,bDone=false;
    int32 RaidBoundsStage=0;
    bool bReloadAttachmentMoved=false,bReloadAttachmentHidden=false;
    int32 FactoryAmmo=0,RaidAmmo=0;
    FVector WalkStart=FVector::ZeroVector;
};
