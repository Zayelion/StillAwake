#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SAData.h"
#include "SAAudio.generated.h"
class UAudioComponent;
class USoundWaveProcedural;
UCLASS()
class STILLAWAKE_API USAAudio : public UActorComponent
{
    GENERATED_BODY()
  public:
    USAAudio();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction *Tick) override;
    void Apply(const FSASettings &Settings, int Rounds);
    void Pause(bool Value);
    void Step();
    void Door();
    void Lamp();
    void SetSleepMix(bool Value);
    void Transition();
    void Dawn(bool Value);
    void Anomalies(float Water, float TV);

  private:
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Channels;
    UPROPERTY() TArray<TObjectPtr<USoundWaveProcedural>> Waves;
    int64 Samples[6] = {};
    float Filters[6] = {}, WaterGain = 0, TVGain = 0;
    uint32 Noise = 1926;
    int StepIndex = 0;
    bool bDawn = false, bSleeping = false;
    void PlayFoley(float Seconds, float Amplitude);
    void Fill(int Channel, float Seconds, float Frequency, float Amplitude, bool Continuous);
};
