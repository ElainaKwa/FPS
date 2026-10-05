#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPSAudioSubsystem.generated.h"

class USoundBase;
class USoundAttenuation;
class USoundConcurrency;
UCLASS()
class MCP_TEST_API UFPSAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    static UFPSAudioSubsystem* Get(const UObject* Context);
    void Play(FName Cue, const FVector& Location = FVector::ZeroVector, bool bLocal = true);
    void PlaySound(USoundBase* Sound, const FVector& Location = FVector::ZeroVector, bool bLocal = true, bool bShot = false);
private:
    bool bRecordingStarted = false;
    UPROPERTY() TMap<FName, TObjectPtr<USoundBase>> Sounds;
    UPROPERTY() TObjectPtr<USoundAttenuation> Attenuation;
    UPROPERTY() TObjectPtr<USoundConcurrency> Concurrency;
};
