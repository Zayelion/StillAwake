#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace SA
{
inline constexpr int CycleCount = 3;

enum class SleepDifficulty
{
    Gentle,
    Manageable,
    Pressured,
    Severe,
    Extreme
};

inline SleepDifficulty SelectSleepDifficulty(double Performance)
{
    if (Performance >= 85)
        return SleepDifficulty::Gentle;
    if (Performance >= 65)
        return SleepDifficulty::Manageable;
    if (Performance >= 45)
        return SleepDifficulty::Pressured;
    if (Performance >= 25)
        return SleepDifficulty::Severe;
    return SleepDifficulty::Extreme;
}

struct HuntResult
{
    int Correct = 0, Missed = 0, FalsePositives = 0;
    double Raw = 0, Performance = 0;
    SleepDifficulty Difficulty = SleepDifficulty::Extreme;
};

enum class Judgment
{
    Rejected,
    Correct,
    FalsePositive
};

class Hunt
{
  public:
    void Reset(const std::vector<bool> &ActiveTargets)
    {
        Active = ActiveTargets;
        Marked.assign(Active.size(), false);
        Correct = 0;
        FalsePositives = 0;
    }

    Judgment Mark(int TargetIndex)
    {
        if (!Contains(TargetIndex) || Marked[TargetIndex])
            return Judgment::Rejected;
        Marked[TargetIndex] = true;
        if (Active[TargetIndex])
        {
            ++Correct;
            return Judgment::Correct;
        }
        ++FalsePositives;
        return Judgment::FalsePositive;
    }

    bool IsMarked(int TargetIndex) const
    {
        return Contains(TargetIndex) && Marked[TargetIndex];
    }

    HuntResult Result(double EmptyPerformance = 65) const
    {
        const int Total = static_cast<int>(std::count(Active.begin(), Active.end(), true));
        const int Missed = Total - Correct;
        const double Raw = Correct - 0.5 * Missed - FalsePositives;
        const double Performance = Total > 0 ? 100 * (Raw + 0.5 * Total) / (1.5 * Total) : EmptyPerformance;
        const double Clamped = std::clamp(Performance, 0.0, 100.0);
        return {Correct, Missed, FalsePositives, Raw, Clamped, SelectSleepDifficulty(Clamped)};
    }

  private:
    bool Contains(int TargetIndex) const
    {
        return TargetIndex >= 0 && static_cast<std::size_t>(TargetIndex) < Active.size();
    }

    std::vector<bool> Active, Marked;
    int Correct = 0, FalsePositives = 0;
};

struct SleepProfile
{
    double AnxietyRiseRate = .10, AnxietyRecoveryRate = .14;
    double ClosetAdvanceRate = .115, ClosetRetreatRate = .22;
    double AnxietyFailureThreshold = 1, ClosetFailureThreshold = 1;
    double WarningSeconds = 1.8, SleepDuration = 18;
};

enum class SleepOutcome
{
    InProgress,
    Anxiety,
    Presence,
    Asleep
};

class Sleep
{
  public:
    void Reset(const SleepProfile &NewProfile)
    {
        Profile = NewProfile;
        AnxietyLevel = .15 * Profile.AnxietyFailureThreshold;
        ClosetLevel = 0;
        RestedSeconds = 0;
        bLampOn = false;
        State = SleepOutcome::InProgress;
    }

    void ToggleLamp()
    {
        if (State == SleepOutcome::InProgress)
            bLampOn = !bLampOn;
    }

    void Advance(double Seconds)
    {
        if (State != SleepOutcome::InProgress || !std::isfinite(Seconds) || Seconds <= 0)
            return;
        if (bLampOn)
        {
            const double TimeToOverload = (Profile.AnxietyFailureThreshold - AnxietyLevel) / Profile.AnxietyRiseRate;
            const double Elapsed = std::min(Seconds, TimeToOverload);
            ClosetLevel = std::max(0.0, ClosetLevel - Elapsed * Profile.ClosetRetreatRate);
            AnxietyLevel = std::min(Profile.AnxietyFailureThreshold, AnxietyLevel + Elapsed * Profile.AnxietyRiseRate);
            if (TimeToOverload <= Seconds)
            {
                AnxietyLevel = Profile.AnxietyFailureThreshold;
                State = SleepOutcome::Anxiety;
            }
            return;
        }

        const double TimeToAttack = (Profile.ClosetFailureThreshold - ClosetLevel) / Profile.ClosetAdvanceRate;
        const double SettlingStarts = std::max(0.0, (AnxietyLevel - .65 * Profile.AnxietyFailureThreshold) /
                                                       Profile.AnxietyRecoveryRate);
        const double SettlingLimit = std::min(.8 * Profile.ClosetFailureThreshold, WarningLevel());
        const double SettlingEnds = (SettlingLimit - ClosetLevel) / Profile.ClosetAdvanceRate;
        const double TimeToSleep = SettlingStarts + Profile.SleepDuration - RestedSeconds;
        const bool FallsAsleep = TimeToSleep <= SettlingEnds && TimeToSleep <= Seconds && TimeToSleep < TimeToAttack;
        const double Elapsed = FallsAsleep ? TimeToSleep : std::min(Seconds, TimeToAttack);
        AnxietyLevel = std::max(0.0, AnxietyLevel - Elapsed * Profile.AnxietyRecoveryRate);
        ClosetLevel = std::min(Profile.ClosetFailureThreshold, ClosetLevel + Elapsed * Profile.ClosetAdvanceRate);
        RestedSeconds += std::max(0.0, std::min(Elapsed, SettlingEnds) - SettlingStarts);
        if (FallsAsleep)
        {
            RestedSeconds = Profile.SleepDuration;
            State = SleepOutcome::Asleep;
            return;
        }
        if (TimeToAttack <= Seconds)
        {
            ClosetLevel = Profile.ClosetFailureThreshold;
            State = SleepOutcome::Presence;
        }
    }

    bool IsLampOn() const { return bLampOn; }
    bool IsWarning() const { return ClosetLevel >= WarningLevel(); }
    double Anxiety() const { return AnxietyLevel / Profile.AnxietyFailureThreshold; }
    double Closet() const { return ClosetLevel / Profile.ClosetFailureThreshold; }
    double Progress() const { return RestedSeconds / Profile.SleepDuration; }
    SleepOutcome Outcome() const { return State; }

  private:
    double WarningLevel() const
    {
        return Profile.ClosetFailureThreshold - Profile.WarningSeconds * Profile.ClosetAdvanceRate;
    }

    SleepProfile Profile;
    double AnxietyLevel = .15, ClosetLevel = 0, RestedSeconds = 0;
    bool bLampOn = false;
    SleepOutcome State = SleepOutcome::InProgress;
};
} // namespace SA
