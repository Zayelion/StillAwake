#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SAData.h"
#include "SABuildInfo.h"
#include "Core/SACycleRules.h"
#include "SAGameMode.generated.h"
class ASAHouse;
class ASACharacter;
class ASAProp;
class ASAShadowEyes;
class USAAudio;
enum class ESAStage
{
    Menu,
    Hunt,
    HuntResult,
    Sleep,
    SleepFailed,
    Paused,
    Ending,
    Error
};
UCLASS()
class STILLAWAKE_API ASAGameMode : public AGameModeBase
{
    GENERATED_BODY()
  public:
    ASAGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY() TObjectPtr<ASAHouse> House;
    UPROPERTY() TObjectPtr<USASaveGame> Save;
    UPROPERTY() TObjectPtr<USAAudio> Audio;
    FSAContent Content;
    FSABuildInfo BuildInfo;
    SA::Hunt Hunt;
    SA::Sleep Sleep;
    SA::HuntResult HuntResult;
    ESAStage Stage = ESAStage::Menu, BeforePause = ESAStage::Hunt;
    bool bSettings = false, bJournal = false, bNewspaper = false;
    float EndingAge = 0;
    int JournalIndex = 0, Cycle = 0;
    FString Feedback, Failure, Prompt, Caption;
    bool CanMove() const
    {
        return Stage == ESAStage::Hunt && !bJournal && !bNewspaper && !bSettings;
    }
    bool CanLook() const { return CanMove() || Stage == ESAStage::Sleep; }
    bool RouteComplete() const;
    void NewGame();
    void Restart();
    void RetrySleep();
    void Title();
    void TogglePause();
    void Escape();
    void Settings();
    void CloseSettings();
    void QuitGame();
    void ObserveMovement(const FVector &Position);
    void Interact();
    FString InteractionTarget(FVector *StickerPoint = nullptr) const;
    static FString NewspaperText();
    void JournalPage(int Direction);
    void ApplySettings();
    void Persist();
    void ApplyAnomaly(int Index);
    FString Guidance() const;
    TArray<FSAStory> UnlockedStories() const;
    FString SaveSlot() const;
    ASACharacter *Character() const;

  private:
    UPROPERTY() TObjectPtr<ASAShadowEyes> ShadowEyes;
    TSet<FString> Inspectables;
    uint32 VisitedCheckpoints = 0;
    float PhaseRemaining = 0, FeedbackRemaining = 0;
    SA::SleepProfile SleepProfile;
    void BeginHunt();
    void FinishHunt();
    void BeginSleep();
    void UpdateSleep(float Delta);
    void UpdateShadowEyes(float Delta);
    void UpdateHuntPrompt();
    void CaptureMouse(bool Capture);
    void BeginEnding();
    void UnlockStory(int CompletedCycles);
};
