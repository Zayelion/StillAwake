#include "SAHUD.h"
#include "SAGameMode.h"
#include "SACharacter.h"
#include "SAAudio.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
void ASAHUD::BeginPlay()
{
    Super::BeginPlay();
    PeripheralHorrors = LoadObject<UTexture2D>(nullptr, TEXT("/Game/StillAwake/Art/T_PeripheralHorrors.T_PeripheralHorrors"));
}

void ASAHUD::DrawGlimpse(const ASAGameMode *Game)
{
    const auto *Player = Game->Character();
    if (!Player || !Game->CanMove())
        return;
    const auto Glimpse = Player->Glimpse(Game->Save->Settings.ReducedIntensity);
    if (Glimpse.Blackout <= 0)
        return;
    DrawRect({0, 0, 0, float(Glimpse.Blackout)}, 0, 0, Canvas->SizeX, Canvas->SizeY);
    if (!PeripheralHorrors || Glimpse.Figure <= 0)
        return;
    constexpr float Heights[] = {.5f, .37f, .43f};
    constexpr float Centers[] = {.53f, .72f, .44f};
    const float Size = FMath::Min(Canvas->SizeY * Heights[Glimpse.Variant], Canvas->SizeX * .30f);
    const float X = Canvas->SizeX * (Glimpse.Right ? .88f : .12f) - .5f * Size;
    const float Y = Canvas->SizeY * Centers[Glimpse.Variant] - .5f * Size;
    DrawTexture(PeripheralHorrors, X, Y, Size, Size, Glimpse.Variant / 3.f + .001f, 0.f, 1.f / 3.f - .002f, 1.f,
                {1, 1, 1, float(Glimpse.Figure)}, BLEND_Translucent);
}
void ASAHUD::Rect(float X, float Y, float W, float H, FLinearColor C)
{
    DrawRect(C, OffsetX + X * Scale, OffsetY + Y * Scale, W * Scale, H * Scale);
}
void ASAHUD::Text(const FString &Value, float X, float Y, float Size, FLinearColor C, float Width)
{
    if (!Canvas || !GEngine)
        return;
    UFont *Font = GEngine->GetLargeFont();
    float SampleWidth, LineHeight;
    GetTextSize(TEXT("Ag"), SampleWidth, LineHeight, Font, 1.f);
    const float Ratio = Size / FMath::Max(1.f, LineHeight);
    TArray<FString> Paragraphs;
    Value.ParseIntoArray(Paragraphs, TEXT("\n"), false);
    float Row = Y;
    for (const FString &P : Paragraphs)
    {
        TArray<FString> Words;
        P.ParseIntoArray(Words, TEXT(" "), true);
        FString Line;
        for (const auto &Word : Words)
        {
            FString Next = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
            float W, H;
            GetTextSize(Next, W, H, Font, Ratio);
            if (W > Width && !Line.IsEmpty())
            {
                DrawText(Line, C, OffsetX + X * Scale, OffsetY + Row * Scale, Font, Ratio * Scale, false);
                Row += Size * 1.45f;
                Line = Word;
            }
            else
                Line = Next;
        }
        DrawText(Line, C, OffsetX + X * Scale, OffsetY + Row * Scale, Font, Ratio * Scale, false);
        Row += Size * 1.45f;
    }
}
void ASAHUD::Button(const FString &Label, float X, float Y, float W, TFunction<void()> Action, bool Enabled)
{
    float MX = 0, MY = 0;
    bool HasMouse = PlayerOwner && PlayerOwner->GetMousePosition(MX, MY);
    FBox2D Bounds({OffsetX + X * Scale, OffsetY + Y * Scale}, {OffsetX + (X + W) * Scale, OffsetY + (Y + 52) * Scale});
    bool Hover = HasMouse && Bounds.IsInside({MX, MY});
    Rect(X, Y, W, 52, Hover && Enabled ? FLinearColor(.22f, .22f, .18f, .95f) : FLinearColor(.1f, .12f, .12f, .94f));
    Text(Label, X + 18, Y + 12, 22, Enabled ? Ink : Muted, W - 36);
    if (Enabled)
        Buttons.Add({Bounds, MoveTemp(Action)});
}
void ASAHUD::Click()
{
    float X, Y;
    if (!PlayerOwner || !PlayerOwner->bShowMouseCursor || !PlayerOwner->GetMousePosition(X, Y))
        return;
    for (const auto &B : Buttons)
        if (B.Bounds.IsInside({X, Y}))
        {
            auto Action = B.Action;
            if (auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>())
                if (G->Audio)
                    G->Audio->Transition();
            Action();
            return;
        }
}
void ASAHUD::Slider(const FString &Label, float X, float Y, float &V, float Min, float Max, float Step, ASAGameMode *G)
{
    Text(Label, X, Y, 22, Ink);
    Text(FString::Printf(TEXT("%.2f"), V), X + 400, Y, 19, Muted, 100);
    Rect(X, Y + 36, 350, 6, FLinearColor(.2f, .21f, .19f));
    Rect(X, Y + 36, 350 * (V - Min) / (Max - Min), 6, Accent);
    float *Value = &V;
    Button(TEXT("-"), X + 380, Y + 29, 55,
           [Value, Min, Step, G]()
           {
               *Value = FMath::Max(Min, *Value - Step);
               G->ApplySettings();
           });
    Button(TEXT("+"), X + 445, Y + 29, 55,
           [Value, Max, Step, G]()
           {
               *Value = FMath::Min(Max, *Value + Step);
               G->ApplySettings();
           });
}
void ASAHUD::Toggle(const FString &Label, float X, float Y, bool &V, ASAGameMode *G)
{
    bool *Value = &V;
    Button((V ? TEXT("[x]  ") : TEXT("[ ]  ")) + Label, X, Y, 510,
           [Value, G]()
           {
               *Value = !*Value;
               G->ApplySettings();
           });
}
void ASAHUD::DrawSettings(ASAGameMode *G)
{
    Rect(0, 0, 1600, 900, FLinearColor(.025f, .032f, .035f, .97f));
    Text(TEXT("SETTINGS"), 150, 60, 44, Ink);
    auto &S = G->Save->Settings;
    Slider(TEXT("Master volume"), 160, 220, S.Master, 0, 1, .05f, G);
    Slider(TEXT("Ambience"), 160, 310, S.Ambience, 0, 1, .05f, G);
    Slider(TEXT("Foley / SFX"), 160, 400, S.SFX, 0, 1, .05f, G);
    Slider(TEXT("Music"), 160, 490, S.Music, 0, 1, .05f, G);
    Slider(TEXT("Interface"), 160, 580, S.UI, 0, 1, .05f, G);
    Slider(TEXT("Mouse sensitivity"), 840, 220, S.Sensitivity, .2f, 4, .1f, G);
    Slider(TEXT("Field of view"), 840, 310, S.FOV, 70, 95, 1, G);
    Slider(TEXT("Brightness"), 840, 400, S.Brightness, .6f, 1.7f, .05f, G);
    Slider(TEXT("Subtitle size"), 840, 490, S.SubtitleScale, .8f, 1.6f, .1f, G);
    Toggle(TEXT("Sound captions"), 840, 577, S.Captions, G);
    Toggle(TEXT("Invert mouse Y"), 840, 635, S.InvertY, G);
    Toggle(TEXT("Walking camera motion"), 840, 693, S.CameraMotion, G);
    Toggle(TEXT("Reduced visual intensity"), 840, 751, S.ReducedIntensity, G);
    for (int I = 0; I < 4; ++I)
    {
        float C = (.035f + I * .035f) * S.Brightness;
        Rect(160 + I * 75, 700, 65, 40, {C, C, C, 1});
    }
    Text(TEXT("All four shades should be distinguishable.\nBrightness also raises the house lighting."), 160, 758, 18,
         Muted, 600);
    Button(TEXT("Back"), 1110, 818, 300, [G]() { G->CloseSettings(); });
}
void ASAHUD::DrawCaption(const FString &Caption, float Size)
{
    Rect(305, 620, 990, 22 + 2.25f * Size, {.02f, .025f, .025f, .8f});
    Text(Caption, 325, 631, Size, Ink, 950);
}
void ASAHUD::DrawSleep(const ASAGameMode *G)
{
    const auto &Sleep = G->Sleep;
    const bool Lamp = Sleep.IsLampOn();
    const bool Warning = Sleep.IsWarning();
    Rect(74, 724, 380, 129, {.02f, .025f, .025f, .78f});
    Text(TEXT("Anxiety"), 94, 738, 19, Ink);
    Rect(94, 773, 340, 4, {.24f, .25f, .23f, 1});
    Rect(94, 773, 340 * Sleep.Anxiety(), 4, Ink);
    Text(TEXT("Falling asleep"), 94, 789, 19, Ink);
    Rect(94, 824, 340, 4, {.24f, .25f, .23f, 1});
    Rect(94, 824, 340 * Sleep.Progress(), 4, Accent);

    Rect(500, 724, 650, 129, {.02f, .025f, .025f, .78f});
    Text(Lamp ? TEXT("E / Click  Switch lamp off") : TEXT("E / Click  Switch lamp on"), 523, 738, 23, Ink, 603);
    const TCHAR *Status = Lamp ? TEXT("The light pushes it back. Anxiety rises.") : TEXT("In the dark, anxiety eases and the closet opens.");
    if (Warning)
        Status = Lamp ? TEXT("The closet is still open.") : TEXT("The shape is close. Turn the lamp on.");
    Text(Status, 523, 787, 20, Ink, 603);

    const auto &Settings = G->Save->Settings;
    if (Settings.Captions && Warning && !Lamp)
    {
        DrawCaption(TEXT("[The closet creaks]"), 20 * Settings.SubtitleScale);
        return;
    }
    if (!G->Feedback.IsEmpty())
        DrawCaption(G->Feedback, 20 * Settings.SubtitleScale);
}
void ASAHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas)
        return;
    Buttons.Reset();
    Scale = FMath::Min(Canvas->SizeX / 1600.f, Canvas->SizeY / 900.f);
    OffsetX = (Canvas->SizeX - 1600 * Scale) / 2;
    OffsetY = (Canvas->SizeY - 900 * Scale) / 2;
    auto *G = GetWorld()->GetAuthGameMode<ASAGameMode>();
    if (!G)
        return;
    if (G->Stage == ESAStage::Error)
    {
        Rect(0, 0, 1600, 900, {.02f, .025f, .025f, .98f});
        Text(TEXT("STILL AWAKE"), 180, 170, 54, Ink);
        Text(G->Failure, 180, 300, 25, Ink, 1150);
        Button(TEXT("Quit"), 180, 500, 350, [G]() { G->QuitGame(); });
        return;
    }
    if (!G->Save)
        return;
    if (G->bSettings)
    {
        DrawSettings(G);
        return;
    }
    if (G->Stage == ESAStage::Menu)
    {
        Rect(0, 0, 1600, 900, {.025f, .032f, .035f, .68f});
        Text(TEXT("T O A D  &  L U C K D R A G O N"), 116, 105, 17, Muted);
        Text(TEXT("S T I L L\nA W A K E"), 110, 180, 84, Ink);
        Text(TEXT("Check the house. Then try to sleep."), 116, 430, 25, Muted);
        Button(TEXT("New game"), 116, 530, 350, [G]() { G->NewGame(); });
        Button(TEXT("Settings"), 116, 594, 350, [G]() { G->Settings(); });
        Button(TEXT("Quit"), 116, 658, 350, [G]() { G->QuitGame(); });
        Text(G->BuildInfo.FormatLabel(), 116, 845, 22, Ink);
        return;
    }
    if (G->Stage == ESAStage::Paused)
    {
        Rect(0, 0, 1600, 900, {.025f, .032f, .035f, .9f});
        Text(TEXT("STILL HERE"), 570, 190, 48, Ink);
        Button(TEXT("Resume"), 575, 315, 450, [G]() { G->TogglePause(); });
        Button(TEXT("Settings"), 575, 382, 450, [G]() { G->Settings(); });
        Button(TEXT("Restart run"), 575, 449, 450, [G]() { G->Restart(); });
        Button(TEXT("Return to title"), 575, 516, 450,
               [G]()
               {
                   G->Persist();
                   G->Title();
               });
        Button(TEXT("Quit"), 575, 583, 450, [G]() { G->QuitGame(); });
        return;
    }
    if (G->Stage == ESAStage::HuntResult)
    {
        DrawRect({0, 0, 0, .82f}, 0, 0, Canvas->SizeX, Canvas->SizeY);
        const TCHAR *Line = TEXT("The house still feels unfamiliar.");
        if (G->HuntResult.Performance >= 65)
            Line = TEXT("The house feels a little more familiar.");
        Text(Line, 414, 395, 30, Ink, 780);
        return;
    }
    if (G->Stage == ESAStage::Sleep)
    {
        DrawSleep(G);
        return;
    }
    if (G->Stage == ESAStage::SleepFailed)
    {
        DrawRect({0, 0, 0, .91f}, 0, 0, Canvas->SizeX, Canvas->SizeY);
        const bool Anxiety = G->Sleep.Outcome() == SA::SleepOutcome::Anxiety;
        Text(Anxiety ? TEXT("The light kept you awake.") : TEXT("The closet opened too far."), 550, 288, 32, Ink, 620);
        Button(TEXT("Try to sleep again"), 570, 412, 460, [G]() { G->RetrySleep(); });
        Button(TEXT("Return to title"), 570, 480, 460, [G]() { G->Title(); });
        return;
    }
    if (G->Stage == ESAStage::Ending)
    {
        DrawRect({0, 0, 0, FMath::Max(.52f, 1 - G->EndingAge * .28f)}, 0, 0, Canvas->SizeX, Canvas->SizeY);
        if (G->EndingAge >= 1)
        {
            Text(G->Content.EndingLine, 354, 330, 34, Ink, 900);
            Button(TEXT("Return to title"), 610, 620, 380, [G]() { G->Title(); });
        }
        return;
    }
    if (G->bNewspaper)
    {
        DrawRect({0, 0, 0, .88f}, 0, 0, Canvas->SizeX, Canvas->SizeY);
        Rect(180, 50, 1240, 800, {.83f, .79f, .66f, 1});
        Text(TEXT("A NOTE TO MYSELF"), 235, 90, 35, {.1f, .08f, .06f}, 1130);
        Rect(235, 145, 1130, 2, {.25f, .21f, .17f});
        Text(G->NewspaperText(), 235, 175, 25, {.08f, .065f, .045f}, 1130);
        Text(TEXT("Click / E / Esc to close"), 235, 798, 20, {.25f, .21f, .17f}, 1130);
        return;
    }
    Rect(798, 448, 4, 4, G->Prompt.IsEmpty() ? FLinearColor(.8f, .78f, .7f, .6f) : Ink);
    if (!G->Prompt.IsEmpty())
    {
        Rect(420, 539, 800, 55, {.02f, .025f, .025f, .72f});
        Text(G->Prompt, 440, 551, 21, Ink, 760);
    }
    if (!G->Feedback.IsEmpty() && G->CanMove())
        DrawCaption(G->Feedback, 20 * G->Save->Settings.SubtitleScale);
    if (!G->Caption.IsEmpty() && G->Save->Settings.Captions && G->CanMove())
    {
        Rect(980, 772, 550, 52, {.02f, .025f, .025f, .72f});
        Text(G->Caption, 1000, 783, 20 * G->Save->Settings.SubtitleScale, Ink, 510);
    }
    FString Guide = G->Guidance();
    if (!Guide.IsEmpty())
    {
        Rect(74, 724, 860, 110, {.02f, .025f, .025f, .72f});
        Text(FString::Printf(TEXT("Night %d of %d"), G->Cycle + 1, SA::CycleCount), 94, 739, 18, Ink, 820);
        Text(Guide, 94, 772, 21, Ink, 820);
    }
    if (G->bJournal)
    {
        auto Stories = G->UnlockedStories();
        if (Stories.IsValidIndex(G->JournalIndex))
        {
            const auto &S = Stories[G->JournalIndex];
            Rect(875, 100, 655, 700, {.06f, .065f, .06f, .97f});
            Text(FString::Printf(TEXT("THE JOURNAL / %d OF %d"), G->JournalIndex + 1, Stories.Num()), 915, 130, 17,
                 Accent);
            Text(S.Title, 915, 200, 34, Ink, 550);
            Text(S.Text, 915, 300, 23, Ink, 550);
            Text(TEXT("E to close / J and K to turn pages"), 915, 745, 18, Muted, 550);
        }
    }
    DrawGlimpse(G);
}
