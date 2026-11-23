#include "SAHouse.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSAHouseSurfaceTest, "StillAwake.Content.HouseSurface",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSAHouseSurfaceTest::RunTest(const FString &Parameters)
{
    auto *Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StillAwake/M_HouseTextured.M_HouseTextured"));
    auto *Atlas = LoadObject<UTexture2D>(nullptr, TEXT("/Game/StillAwake/Art/T_HouseSurfaces.T_HouseSurfaces"));
    auto *Sprites = LoadObject<UTexture2D>(nullptr, TEXT("/Game/StillAwake/Art/T_PeripheralHorrors.T_PeripheralHorrors"));
    if (!TestNotNull(TEXT("Textured house material is prepared"), Material) ||
        !TestNotNull(TEXT("House surface atlas is prepared"), Atlas) ||
        !TestNotNull(TEXT("Peripheral sprites are prepared"), Sprites))
        return false;

    // NullRHI automation loads source data without building a runtime texture resource.
    TestTrue(TEXT("House atlas is configured for mipmapped power-of-two cooking"),
             Atlas->MipGenSettings == TMGS_SimpleAverage &&
                 Atlas->PowerOfTwoMode == ETexturePowerOfTwoSetting::ResizeToSpecificResolution &&
                 Atlas->ResizeDuringBuildX == 1024 && Atlas->ResizeDuringBuildY == 1024);
    const int64 SpriteWidth = Sprites->Source.GetSizeX(), SpriteHeight = Sprites->Source.GetSizeY();
    TestTrue(TEXT("Peripheral sprites retain three square cells"), SpriteHeight > 0 && SpriteWidth == 3 * SpriteHeight);
    TestTrue(TEXT("Peripheral sprites cook as unmipped UI art with alpha"),
             Sprites->CompressionSettings == TC_EditorIcon && !Sprites->CompressionNoAlpha &&
                 Sprites->MipGenSettings == TMGS_NoMipmaps && Sprites->LODGroup == TEXTUREGROUP_UI);
    TArray64<uint8> Pixels;
    if (TestTrue(TEXT("Peripheral sprite source has four channels"),
                 Sprites->Source.GetFormat() == TSF_BGRA8 && Sprites->Source.GetMipData(Pixels, 0) &&
                     Pixels.Num() > 0 && Pixels.Num() == SpriteWidth * SpriteHeight * 4))
    {
        bool bVisiblePixel = false;
        for (int64 Index = 3; Index < Pixels.Num() && !bVisiblePixel; Index += 4)
            bVisiblePixel = Pixels[Index] > 0;
        TestTrue(TEXT("Peripheral sprite source contains artwork on a transparent background"),
                 bVisiblePixel && Pixels[3] == 0 && Pixels[SpriteWidth * 4 - 1] == 0 &&
                     Pixels[(SpriteHeight - 1) * SpriteWidth * 4 + 3] == 0 && Pixels.Last() == 0);
    }
    TestTrue(TEXT("House surface supports the instanced floor mesh"),
             Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
    FLinearColor Color;
    TestTrue(TEXT("Appearance can still be changed by tint anomalies"),
             Material->GetVectorParameterValue(FName(TEXT("Tint")), Color));
    TestTrue(TEXT("Grain scale is adjustable by surface type"),
             Material->GetVectorParameterValue(FName(TEXT("GrainScale")), Color));
    UTexture *MaterialAtlas = nullptr;
    TestTrue(TEXT("House material samples the generated atlas"),
             Material->GetTextureParameterValue(FName(TEXT("SurfaceAtlas")), MaterialAtlas) && MaterialAtlas == Atlas);
    for (const TCHAR *Name : {TEXT("Roughness"), TEXT("Metallic"), TEXT("Emission"), TEXT("SurfaceDetail")})
    {
        float Value = 0;
        TestTrue(FString(Name) + TEXT(" is adjustable by surface type"),
                 Material->GetScalarParameterValue(FName(Name), Value));
    }

    UWorld *World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world created"), World))
        return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto *House = World->SpawnActor<ASAHouse>();
    House->Build();
    TestTrue(TEXT("House build uses the textured parent"), House->Surface.Get() == Material);
    TSet<FIntPoint> Tiles;
    for (const auto &Entry : House->Materials)
    {
        const UMaterialInstanceDynamic *Instance = Entry.Value.Get();
        FLinearColor Tile;
        float Detail = 0;
        if (TestTrue(Entry.Key + TEXT(" samples a tiled finish"),
                     Instance->GetVectorParameterValue(FName(TEXT("AtlasTile")), Tile) &&
                         Instance->GetScalarParameterValue(FName(TEXT("SurfaceDetail")), Detail) && Detail > 0 &&
                         Tile.R >= 0 && Tile.R < 4 && Tile.G >= 0 && Tile.G < 4))
            Tiles.Add(FIntPoint(FMath::RoundToInt(Tile.R), FMath::RoundToInt(Tile.G)));
    }
    TestEqual(TEXT("The house palette uses all sixteen authored finishes"), Tiles.Num(), 16);
    return !HasAnyErrors();
}
#endif
