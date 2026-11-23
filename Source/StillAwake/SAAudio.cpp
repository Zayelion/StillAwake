#include "SAAudio.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
USAAudio::USAAudio()
{
    PrimaryComponentTick.bCanEverTick = true;
}
void USAAudio::BeginPlay()
{
    Super::BeginPlay();
    for (int I = 0; I < 6; ++I)
    {
        auto *W = NewObject<USoundWaveProcedural>(this);
        W->SetSampleRate(22050);
        W->NumChannels = 1;
        W->Duration = INDEFINITELY_LOOPING_DURATION;
        W->bLooping = false;
        W->SoundGroup = SOUNDGROUP_Default;
        Waves.Add(W);
        auto *C = NewObject<UAudioComponent>(GetOwner());
        C->bAutoActivate = false;
        C->bIsUISound = true;
        C->SetSound(W);
        C->RegisterComponent();
        Channels.Add(C);
    }
    Fill(0, 1, 150, .008f, true);
    Fill(1, 1, 0, .055f, true);
    Fill(4, 1, 0, .14f, true);
    Fill(5, 1, 0, .025f, true);
    Channels[4]->SetVolumeMultiplier(0);
    Channels[5]->SetVolumeMultiplier(0);
    for (int I : {0, 1, 4, 5})
        Channels[I]->Play();
}
void USAAudio::Fill(int Channel, float Seconds, float Frequency, float Amplitude, bool Continuous)
{
    if (!Waves.IsValidIndex(Channel))
        return;
    int Count = int(22050 * Seconds);
    TArray<int16> PCM;
    PCM.SetNumUninitialized(Count);
    for (int I = 0; I < Count; ++I)
    {
        Noise = Noise * 1664525u + 1013904223u;
        float N = float(Noise >> 8) / 8388607.5f - 1;
        Filters[Channel] = FMath::Lerp(Filters[Channel], N, Channel == 4 ? .3f : .06f);
        double T = double(Samples[Channel]++) / 22050;
        float Local = float(I) / 22050;
        // Soft attack/release, filtered noise: no hard low-frequency footstep thump.
        float Envelope =
            Continuous ? 1.f : FMath::Sin(PI * FMath::Clamp(Local / Seconds, 0.f, 1.f)) * FMath::Exp(-Local * 8);
        float Signal = Filters[Channel];
        if (Channel == 1)
        {
            // Original sparse C-minor melody and slow pad. Continuous time keeps buffer seams smooth.
            const double Notes[] = {130.8128, 155.5635, 195.9977, 233.0819, 207.6523, 195.9977, 155.5635, 146.8324};
            const double Beat = 2.8, Phase = FMath::Fmod(T, Beat) / Beat;
            double F = Notes[int(T / Beat) % 8];
            double Bell = FMath::Sin(PI * Phase) * FMath::Exp(-Phase * 3);
            Signal = float(.55 * Bell * (FMath::Sin(2 * PI * F * T) + .23 * FMath::Sin(2 * PI * F * 2 * T)) +
                           .14 * FMath::Sin(2 * PI * 65.4064 * T) * (.7 + .3 * FMath::Sin(T * .17)) +
                           .09 * FMath::Sin(2 * PI * 97.9989 * T));
        }
        else if (Channel == 4)
            Signal = Filters[Channel] * (.7f + .3f * FMath::Sin(T * 31));
        else if (Channel == 3)
            Signal = .6f * Signal + .4f * FMath::Sin(T * Frequency * 2 * PI);
        PCM[I] = int16(FMath::Clamp(Signal * Amplitude * Envelope, -1.f, 1.f) * 32767);
    }
    Waves[Channel]->QueueAudio(reinterpret_cast<const uint8 *>(PCM.GetData()), PCM.Num() * sizeof(int16));
}
void USAAudio::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction *Tick)
{
    Super::TickComponent(Delta, Type, Tick);
    if (Waves.Num() < 6)
        return;
    for (int I : {0, 1, 4, 5})
        if (Waves[I]->GetAvailableAudioByteCount() < 22050)
            Fill(I, 1, 150, I == 0 ? .008f : I == 1 ? .055f : I == 4 ? .14f : .025f, true);
}
void USAAudio::Apply(const FSASettings &S, int Rounds)
{
    if (Channels.Num() < 6)
        return;
    Channels[0]->SetVolumeMultiplier(bDawn ? 0 : S.Master * S.Ambience * (bSleeping ? .5f : 1.f));
    Channels[1]->SetVolumeMultiplier(bDawn || bSleeping ? 0 : S.Master * S.Music);
    Channels[2]->SetVolumeMultiplier(S.Master * S.SFX);
    Channels[3]->SetVolumeMultiplier(S.Master * S.UI);
    Channels[4]->SetVolumeMultiplier(bDawn || bSleeping ? 0 : S.Master * S.SFX * WaterGain);
    Channels[5]->SetVolumeMultiplier(bDawn || bSleeping ? 0 : S.Master * S.SFX * TVGain);
}
void USAAudio::Anomalies(float Water, float TV)
{
    WaterGain = FMath::Clamp(Water, 0.f, 1.f);
    TVGain = FMath::Clamp(TV, 0.f, 1.f);
}
void USAAudio::Pause(bool Value)
{
    for (auto C : Channels)
        C->SetPaused(Value);
}
void USAAudio::Step()
{
    PlayFoley(.24f, .045f);
}
void USAAudio::Door()
{
    PlayFoley(.5f, .06f);
}
void USAAudio::Lamp()
{
    PlayFoley(.09f, .11f);
}
void USAAudio::SetSleepMix(bool Value)
{
    bSleeping = Value;
}
void USAAudio::PlayFoley(float Seconds, float Amplitude)
{
    if (Waves.Num() < 6)
        return;
    Channels[2]->Stop();
    Waves[2]->ResetAudio();
    Fill(2, Seconds, 0, Amplitude, false);
    Channels[2]->Play();
}
void USAAudio::Transition()
{
    if (Waves.Num() < 6)
        return;
    Channels[3]->Stop();
    Waves[3]->ResetAudio();
    Fill(3, 1.2f, 78, .025f, false);
    Channels[3]->Play();
}
void USAAudio::Dawn(bool Value)
{
    bDawn = Value;
}
