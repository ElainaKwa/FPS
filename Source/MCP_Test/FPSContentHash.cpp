#include "FPSContentHash.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/SecureHash.h"

namespace FPSContent
{
FString Hash(const TArray<uint8>& Bytes)
{
    uint8 Digest[FSHA1::DigestSize];
    FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Digest);
    return BytesToHex(Digest, UE_ARRAY_COUNT(Digest));
}
#if WITH_EDITOR
FString ResourceRevision(const FSoftObjectPath& Path, TArray<FString>& Errors)
{
    auto& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        TArray<FName> Pending{Path.GetLongPackageFName()};
        TSet<FName> Seen;
        TArray<FString> Revisions;
        while (!Pending.IsEmpty())
        {
            const FName Name = Pending.Pop();
            if (Seen.Contains(Name) || Name.ToString().StartsWith(TEXT("/Script/"))) continue;
            Seen.Add(Name);
            const auto Package = Registry.GetAssetPackageDataCopy(Name);
            if (!Package.IsSet() || Package->GetPackageSavedHash().IsZero())
            {
                Errors.Add(TEXT("Missing saved resource revision: ") + Name.ToString());
                continue;
            }
            Revisions.Add(LexToString(Package->GetPackageSavedHash()));
            TArray<FName> Dependencies;
            Registry.GetDependencies(Name, Dependencies);
            Pending.Append(Dependencies);
        }
        Revisions.Sort();
        return FString::Join(Revisions, TEXT(";"));
}
#endif

}

