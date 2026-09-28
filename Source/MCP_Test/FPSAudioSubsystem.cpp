#include "FPSAudioSubsystem.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"

void UFPSAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    for (const TCHAR* Name : {TEXT("Fire_AK"), TEXT("Fire_M4"), TEXT("Fire_MP5"), TEXT("Fire_UMP45"),
        TEXT("Fire_M9"), TEXT("Fire_HeavyPistol"), TEXT("ReloadStart"), TEXT("ReloadEnd"),
        TEXT("Equip"), TEXT("Inspect"), TEXT("Empty"), TEXT("Hit"), TEXT("Kill")})
    {
        const FString Path = FString::Printf(TEXT("/Game/FPSDemo/Audio/S_%s.S_%s"), Name, Name);
        if (auto* Sound = LoadObject<USoundBase>(nullptr, *Path)) Sounds.Add(FName(Name), Sound);
        else UE_LOG(LogTemp, Error, TEXT("FPS_AUDIO missing %s"), *Path);
    }
    Attenuation = NewObject<USoundAttenuation>(this);
    Attenuation->Attenuation.bAttenuate = true;
    Attenuation->Attenuation.bSpatialize = true;
    Attenuation->Attenuation.AttenuationShapeExtents = FVector(200);
    Attenuation->Attenuation.FalloffDistance = 4000;
    Concurrency = NewObject<USoundConcurrency>(this);
    Concurrency->Concurrency.MaxCount = 24;
    Concurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopOldest;
}

UFPSAudioSubsystem* UFPSAudioSubsystem::Get(const UObject* Context)
{
    auto* World = Context ? Context->GetWorld() : nullptr;
    return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UFPSAudioSubsystem>() : nullptr;
}

void UFPSAudioSubsystem::Play(FName Cue, const FVector& Location, bool bLocal)
{
    auto* World = GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer) return;
    const auto* Found = Sounds.Find(Cue);
    if (!Found) return;
#if !UE_BUILD_SHIPPING
    if (!bRecordingStarted && FParse::Param(FCommandLine::Get(), TEXT("FPSAudioTrace")))
    {
        bRecordingStarted = true;
        FApp::SetUnfocusedVolumeMultiplier(1.f);
        UAudioMixerBlueprintLibrary::StartRecordingOutput(World, 20.f);
        FTimerHandle Timer;
        TWeakObjectPtr<UFPSAudioSubsystem> WeakThis(this);
        World->GetTimerManager().SetTimer(Timer, [WeakThis]()
        {
            if (auto* Self = WeakThis.Get())
            {
                const FString Name = Self->GetWorld()->GetNetMode() == NM_Client ? TEXT("Audio-client") : TEXT("Audio-server");
                UAudioMixerBlueprintLibrary::StopRecordingOutput(Self, EAudioRecordingExportType::WavFile, Name,
                    FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("FPSFlowTests")));
                UE_LOG(LogTemp, Display, TEXT("FPS_AUDIO RECORD exported %s"), *Name);
            }
        }, 20.f, false);
    }
#endif
    const bool bShot = Cue.ToString().StartsWith(TEXT("Fire_"));
    const float Volume = bShot ? .55f : .35f;
    UAudioComponent* Voice = bLocal
        ? UGameplayStatics::SpawnSound2D(World, *Found, Volume, 1, 0, Concurrency)
        : UGameplayStatics::SpawnSoundAtLocation(World, *Found, Location, FRotator::ZeroRotator, Volume, 1, 0, Attenuation, Concurrency);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("FPSAudioTrace")))
        UE_LOG(LogTemp, Display, TEXT("FPS_AUDIO %s cue=%s route=%s net=%d active=%d"), Voice && Voice->IsPlaying() ? TEXT("PASS") : TEXT("FAIL"),
            *Cue.ToString(), bLocal ? TEXT("LOCAL") : TEXT("SPATIAL"), int32(World->GetNetMode()), Voice && Voice->IsPlaying());
#endif
}

