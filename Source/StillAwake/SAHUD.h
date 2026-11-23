#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SAHUD.generated.h"
class ASAGameMode;
class UTexture2D;
UCLASS()
class STILLAWAKE_API ASAHUD : public AHUD
{
    GENERATED_BODY()
  public:
    virtual void BeginPlay() override;
    virtual void DrawHUD() override;
    void Click();

  private:
    struct FButton
    {
        FBox2D Bounds;
        TFunction<void()> Action;
    };
    TArray<FButton> Buttons;
    UPROPERTY() TObjectPtr<UTexture2D> PeripheralHorrors;
    float Scale = 1, OffsetX = 0, OffsetY = 0;
    FLinearColor Ink = FLinearColor(.88f, .85f, .75f), Muted = FLinearColor(.56f, .57f, .53f),
                 Accent = FLinearColor(.62f, .46f, .26f);
    void Rect(float X, float Y, float W, float H, FLinearColor Color);
    void Text(const FString &Value, float X, float Y, float Size, FLinearColor Color, float Width = 1300);
    void Button(const FString &Label, float X, float Y, float Width, TFunction<void()> Action, bool Enabled = true);
    void Slider(const FString &Label, float X, float Y, float &Value, float Min, float Max, float Step,
                ASAGameMode *Game);
    void Toggle(const FString &Label, float X, float Y, bool &Value, ASAGameMode *Game);
    void DrawSettings(ASAGameMode *Game);
    void DrawSleep(const ASAGameMode *Game);
    void DrawCaption(const FString &Caption, float Size);
    void DrawGlimpse(const ASAGameMode *Game);
};
