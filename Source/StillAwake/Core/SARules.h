#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace SA
{
enum class Outcome
{
    CorrectClean,
    CorrectAnomaly,
    MissedAnomaly,
    FalseAlarm
};
inline bool Correct(Outcome Result)
{
    return Result == Outcome::CorrectClean || Result == Outcome::CorrectAnomaly;
}
enum class ExitChoice
{
    Front,
    Back
};
struct DoorRound
{
    bool Anomaly = false, Entered = false, Resolved = false, Paused = false;
    explicit DoorRound(bool InAnomaly = false) : Anomaly(InAnomaly) {}
    void Enter()
    {
        if (!Paused && !Resolved)
            Entered = true;
    }
    std::optional<Outcome> Choose(ExitChoice Door)
    {
        if (Paused || Resolved || !Entered)
            return {};
        Resolved = true;
        if (Door == ExitChoice::Back)
            return Anomaly ? Outcome::MissedAnomaly : Outcome::CorrectClean;
        return Anomaly ? Outcome::CorrectAnomaly : Outcome::FalseAlarm;
    }
};
struct Statistics
{
    int Streak = 0, Best = 0, Rounds = 0, CorrectRounds = 0, Tutorial = 0;
    bool EndingSeen = false;
    int Remaining(int Target = 10) const
    {
        return std::max(0, Target - Streak);
    }
    bool NeedsSafeRound() const
    {
        return Streak == 0;
    }
    bool Resolve(Outcome Result, int Target = 10)
    {
        ++Rounds;
        if (Correct(Result))
        {
            ++Streak;
            ++CorrectRounds;
            Best = std::max(Best, Streak);
            Tutorial = std::min(2, Tutorial + 1);
        }
        else
            Streak = 0;
        bool Won = Streak >= Target;
        EndingSeen |= Won;
        return Won;
    }
};
struct Candidate
{
    std::string Id;
    int Room = 0, Tier = 1, MinProgress = 0;
    double Weight = 1;
};
class Selector
{
    uint32_t StateValue;
    std::vector<std::optional<Candidate>> History;
    double Random01()
    {
        StateValue ^= StateValue << 13;
        StateValue ^= StateValue >> 17;
        StateValue ^= StateValue << 5;
        return StateValue / 4294967296.0;
    }

  public:
    explicit Selector(uint32_t Seed = 1) : StateValue(Seed ? Seed : 1) {}
    uint32_t State() const
    {
        return StateValue;
    }
    void Remember(std::optional<Candidate> Item)
    {
        History.push_back(Item);
        if (History.size() > 3)
            History.erase(History.begin());
    }
    std::string Next(const std::vector<Candidate> &Pool, int Streak, int Progress, double Chance = .65)
    {
        if (Random01() >= Chance)
        {
            Remember({});
            return {};
        }
        int Tier = Streak < 3 ? 1 : Streak < 6 ? 2 : 3;
        std::vector<Candidate> Eligible, NoId, Available;
        for (const auto &Item : Pool)
        {
            if (Item.Tier > Tier || Item.MinProgress > Progress || Item.Weight <= 0 || !std::isfinite(Item.Weight))
                continue;
            Eligible.push_back(Item);
            bool RecentId = false, RecentRoom = false;
            for (size_t I = 0; I < History.size(); ++I)
                if (History[I])
                {
                    RecentId |= History[I]->Id == Item.Id;
                    RecentRoom |= I + 2 >= History.size() && History[I]->Room == Item.Room;
                }
            if (!RecentId)
            {
                NoId.push_back(Item);
                if (!RecentRoom)
                    Available.push_back(Item);
            }
        }
        if (Available.empty())
            Available = NoId;
        if (Available.empty())
            Available = Eligible;
        if (Available.empty())
        {
            Remember({});
            return {};
        }
        double Sum = 0;
        for (const auto &Item : Available)
            Sum += Item.Weight;
        double Remaining = Random01() * Sum;
        Candidate Chosen = Available.back();
        for (const auto &Item : Available)
        {
            Remaining -= Item.Weight;
            if (Remaining < 0)
            {
                Chosen = Item;
                break;
            }
        }
        Remember(Chosen);
        return Chosen.Id;
    }
};
} // namespace SA
