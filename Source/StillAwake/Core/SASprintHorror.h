#pragma once
#include <algorithm>
#include <cmath>

namespace SA
{
struct GlimpseFrame
{
    double Blackout = 0, Figure = 0;
    int Variant = 0;
    bool Right = false;
};

class SprintHorror
{
  public:
    void Advance(double Seconds, bool MovingFast)
    {
        if (!std::isfinite(Seconds) || Seconds <= 0)
            return;
        const double PulseTime = std::min(Seconds, Duration - Age);
        Age += PulseTime;
        Seconds -= PulseTime;
        if (!MovingFast)
        {
            Fill = std::max(0.0, Fill - Seconds * .65);
            return;
        }
        Fill = std::min(1.0, Fill + Seconds / FillTime);
        if (Fill < 1)
            return;
        Fill = 0;
        Age = 0;
        Current = Next;
        Next = (Next + 1) % 6;
    }

    void Interrupt()
    {
        Fill = 0;
        Age = Duration;
    }

    GlimpseFrame Frame(bool ReducedIntensity = false) const
    {
        if (Age >= Duration)
            return {};
        const double Darkness = std::clamp((Duration - Age) / .16, 0.0, 1.0);
        const double Figure = std::clamp((Age - .035) / .045, 0.0, 1.0) *
                              std::clamp((.255 - Age) / .09, 0.0, 1.0);
        return {Darkness * (ReducedIntensity ? .42 : 1), Figure * (ReducedIntensity ? .58 : 1),
                Current % 3, Current % 2 != 0};
    }

  private:
    static constexpr double FillTime = 1.5, Duration = .28;
    double Fill = 0, Age = Duration;
    int Current = 0, Next = 0;
};
} // namespace SA
