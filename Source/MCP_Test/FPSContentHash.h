#pragma once
#include "CoreMinimal.h"

namespace FPSContent
{
    FString Hash(const TArray<uint8>& Bytes);
#if WITH_EDITOR
    FString ResourceRevision(const FSoftObjectPath& Path, TArray<FString>& Errors);
#endif
}
