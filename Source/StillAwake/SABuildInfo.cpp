#include "SABuildInfo.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FSABuildInfo FSABuildInfo::Load(const FString &Path)
{
    FSABuildInfo Local;
    GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Local.Version,
                       GGameIni);
    Local.BuildVersion = Local.Version;
    FString Text;
    const FString File = Path.IsEmpty() ? FPaths::ProjectDir() / TEXT("build-info.json") : Path;
    if (!FFileHelper::LoadFileToString(Text, *File, FFileHelper::EHashOptions::None, FILEREAD_Silent))
        return Local;
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root)
        return Local;
    FSABuildInfo Info;
    const TPair<const TCHAR *, FString *> Fields[] = {
        {TEXT("version"), &Info.Version}, {TEXT("buildNumber"), &Info.BuildNumber},
        {TEXT("buildVersion"), &Info.BuildVersion}, {TEXT("runId"), &Info.RunId},
        {TEXT("configuration"), &Info.Configuration}};
    for (const auto &Field : Fields)
    {
        const auto Value = Root->TryGetField(Field.Key);
        if (!Value || Value->Type != EJson::String || !Value->TryGetString(*Field.Value) || Field.Value->IsEmpty())
            return Local;
    }
    return Info;
}

FString FSABuildInfo::FormatLabel() const
{
    return FString::Printf(TEXT("v%s / Build %s"), *Version, *BuildNumber);
}
