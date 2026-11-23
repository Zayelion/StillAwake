#pragma once
#include "CoreMinimal.h"

struct FSABuildInfo
{
    FString Version;
    FString BuildNumber = TEXT("local");
    FString BuildVersion;
    FString RunId = TEXT("local");
    FString Configuration = TEXT("local");

    static FSABuildInfo Load(const FString &Path = FString());
    FString FormatLabel() const;
};
