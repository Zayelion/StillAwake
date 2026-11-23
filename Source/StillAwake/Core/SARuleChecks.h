#pragma once
#include "SACycleRules.h"
#include "SARules.h"
#include "SASprintHorror.h"
namespace SA
{
struct CheckResult
{
    int Count = 0;
    std::vector<std::string> Errors;
};
inline CheckResult RunChecks()
{
    CheckResult R;
    auto Check = [&](bool Valid, const std::string &Name)
    {
        ++R.Count;
        if (!Valid)
            R.Errors.push_back(Name);
    };
    for (bool Anomaly : {false, true})
        for (auto Door : {ExitChoice::Front, ExitChoice::Back})
        {
            DoorRound Round(Anomaly);
            Check(!Round.Choose(Door).has_value() && !Round.Resolved, "Cannot score before entering");
            Round.Enter();
            auto Expected = Door == ExitChoice::Back ? (Anomaly ? Outcome::MissedAnomaly : Outcome::CorrectClean)
                                                     : (Anomaly ? Outcome::CorrectAnomaly : Outcome::FalseAlarm);
            Check(Round.Choose(Door) == Expected, "Door/anomaly truth table");
            Check(!Round.Choose(Door).has_value(), "One score per house");
        }
    DoorRound Paused(true);
    Paused.Paused = true;
    Paused.Enter();
    Check(!Paused.Entered, "Paused porch cannot enter");
    Paused.Paused = false;
    Paused.Enter();
    Paused.Paused = true;
    Check(!Paused.Choose(ExitChoice::Front) && !Paused.Resolved, "Pause freezes door decision");
    Paused.Paused = false;
    Check(Paused.Choose(ExitChoice::Front) == Outcome::CorrectAnomaly, "Resume preserves anomaly");
    Statistics S;
    Check(S.Remaining() == 10 && S.NeedsSafeRound(), "New run begins at ten with safe house");
    for (int I = 1; I <= 9; ++I)
    {
        Check(!S.Resolve(I % 2 ? Outcome::CorrectClean : Outcome::CorrectAnomaly) && S.Remaining() == 10 - I,
              "Countdown without early ending");
    }
    Check(S.Resolve(Outcome::CorrectClean) && S.Remaining() == 0, "Tenth correct choice wins");
    S.Resolve(Outcome::FalseAlarm);
    Check(S.Remaining() == 10 && S.NeedsSafeRound(), "Wrong front exit resets count and safe round");
    Check(S.CorrectRounds == 10 && S.Rounds == 11 && S.Best == 10 && S.EndingSeen, "Lifetime progress survives reset");
    S.Resolve(Outcome::CorrectClean);
    S.Resolve(Outcome::MissedAnomaly);
    Check(S.Remaining() == 10 && S.NeedsSafeRound(), "Wrong back exit resets count and safe round");
    std::vector<Candidate> Pool;
    for (int I = 0; I < 10; ++I)
        Pool.push_back({"A" + std::to_string(I), I < 3 ? 1 : I < 6 ? 2 : 3, 1, 0, 1});
    Selector A(1926), B(1926);
    bool Same = true;
    int Anomalous = 0;
    std::vector<std::string> Seen;
    for (int I = 0; I < 10000; ++I)
    {
        auto Id = A.Next(Pool, 1, 0);
        Same &= Id == B.Next(Pool, 1, 0);
        Anomalous += !Id.empty();
        if (!Id.empty() && std::find(Seen.begin(), Seen.end(), Id) == Seen.end())
            Seen.push_back(Id);
    }
    Check(Same, "Deterministic ten-thousand-round stream");
    Check(Anomalous > 6200 && Anomalous < 6800, "65 percent anomaly frequency");
    Check(Seen.size() == 10, "All ten anomalies eligible from second round");
    Selector Recent(7);
    std::vector<std::string> Ids;
    std::vector<int> Rooms;
    bool Cooldown = true;
    for (int I = 0; I < 100; ++I)
    {
        auto Id = Recent.Next(Pool, 1, 0, 1);
        int N = std::stoi(Id.substr(1)), Room = N < 3 ? 1 : N < 6 ? 2 : 3;
        Cooldown &= std::find(Ids.begin(), Ids.end(), Id) == Ids.end() &&
                    std::find(Rooms.begin(), Rooms.end(), Room) == Rooms.end();
        Ids.push_back(Id);
        Rooms.push_back(Room);
        if (Ids.size() > 3)
            Ids.erase(Ids.begin());
        if (Rooms.size() > 2)
            Rooms.erase(Rooms.begin());
    }
    Check(Cooldown, "Recent IDs and room cooldowns");
    Check(Selector(1).Next({}, 0, 0, 1).empty(), "Empty pool is clean");
    Selector Small(2);
    std::vector<Candidate> One = {{"one", 1, 1, 0, 1}};
    Small.Next(One, 0, 0, 1);
    Check(Small.Next(One, 0, 0, 1) == "one", "Small pool fallback");
    Check(!Selector(0).Next(Pool, 1, 0, 1).empty(), "Zero seed is normalized");
    Check(Selector(1).Next(Pool, 1, 0, 0).empty(), "Zero probability forces clean");
    Check(Selector(1).Next({{"disabled", 1, 1, 0, 0}}, 1, 0, 1).empty(), "Disabled candidate excluded");

    Hunt Inspection;
    Inspection.Reset({true, true, true, true, true, true, false, false});
    Check(Inspection.Result().Missed == 6 && Inspection.Result().Raw == -3 && Inspection.Result().Performance == 0,
          "All misses normalize to zero");
    Check(Inspection.Mark(-1) == Judgment::Rejected && Inspection.Mark(8) == Judgment::Rejected,
          "Unknown inspection targets cannot be judged");
    for (int Target = 0; Target < 4; ++Target)
        Check(Inspection.Mark(Target) == Judgment::Correct && Inspection.IsMarked(Target), "Anomaly is scored and marked");
    Check(Inspection.Mark(0) == Judgment::Rejected && Inspection.Result().Correct == 4, "Duplicate anomaly is rejected");
    Check(Inspection.Result().Raw == 3 && std::abs(Inspection.Result().Performance - 200.0 / 3) < .001,
          "Four correct and two misses use the stated formula");
    Check(Inspection.Mark(6) == Judgment::FalsePositive && Inspection.IsMarked(6), "Normal target is scored and marked");
    Check(Inspection.Mark(6) == Judgment::Rejected && Inspection.Result().FalsePositives == 1,
          "Duplicate false positive is rejected");
    Check(Inspection.Result().Raw == 2 && std::abs(Inspection.Result().Performance - 500.0 / 9) < .001,
          "False positive deducts one raw point");
    Inspection.Reset({true, true});
    Check(!Inspection.IsMarked(0) && Inspection.Result().FalsePositives == 0 && Inspection.Result().Correct == 0,
          "New hunt starts with a fresh record");
    Inspection.Mark(0);
    Inspection.Mark(1);
    Check(Inspection.Result().Performance == 100 && Inspection.Result().Difficulty == SleepDifficulty::Gentle,
          "Perfect hunt gives Gentle regardless of size");
    Inspection.Reset({false});
    Check(Inspection.Result(70).Performance == 70 && Inspection.Result(70).Difficulty == SleepDifficulty::Manageable,
          "Authored empty-hunt fallback avoids division by zero");
    Check(SelectSleepDifficulty(84.99) == SleepDifficulty::Manageable && SelectSleepDifficulty(85) == SleepDifficulty::Gentle &&
              SelectSleepDifficulty(64.99) == SleepDifficulty::Pressured && SelectSleepDifficulty(65) == SleepDifficulty::Manageable &&
              SelectSleepDifficulty(44.99) == SleepDifficulty::Severe && SelectSleepDifficulty(45) == SleepDifficulty::Pressured &&
              SelectSleepDifficulty(24.99) == SleepDifficulty::Extreme && SelectSleepDifficulty(25) == SleepDifficulty::Severe,
          "Sleep difficulty uses the exact grade boundaries");

    const SleepProfile Profiles[] = {{.085, .155, .095, .24, 1, 1, 2, 16},
                                     {.10, .14, .115, .22, 1, 1, 1.8, 18},
                                     {.115, .1275, .132, .21, 1, 1, 1.5, 19},
                                     {.128, .115, .149, .201, 1, 1, 1.2, 20},
                                     {.14, .105, .162, .195, 1, 1, 1, 21}};
    for (const SleepProfile &Base : Profiles)
    {
        SleepProfile Profile = Base;
        Profile.AnxietyRiseRate *= 1.08;
        Profile.ClosetAdvanceRate *= 1.08;
        Sleep Bed;
        Bed.Reset(Profile);
        Bed.Advance(Profile.ClosetFailureThreshold / Profile.ClosetAdvanceRate - Profile.WarningSeconds + .01);
        Check(Bed.IsWarning() && Bed.Outcome() == SleepOutcome::InProgress && Bed.Progress() > 0,
              "Darkness gives time to react before presence failure");
        const double Threat = Bed.Closet(), Anxiety = Bed.Anxiety(), Progress = Bed.Progress();
        Bed.ToggleLamp();
        Bed.Advance(.25);
        Check(Bed.IsLampOn() && Bed.Closet() < Threat && Bed.Closet() > 0 && Bed.Anxiety() > Anxiety &&
                  Bed.Progress() == Progress,
              "Light gradually retreats the threat and raises anxiety without advancing sleep");
        Bed.Advance(20);
        Check(Bed.Outcome() == SleepOutcome::Anxiety && Bed.Anxiety() == 1, "Leaving the lamp on causes overload");
        Bed.Reset(Profile);
        Check(!Bed.IsLampOn() && Bed.Anxiety() == .15 && Bed.Closet() == 0 && Bed.Progress() == 0,
              "Retry resets sleep meters and lamp");
        Bed.Advance(30);
        Check(Bed.Outcome() == SleepOutcome::Presence && Bed.Closet() == 1, "Leaving the lamp off causes presence failure");
        Bed.Reset(Profile);
        for (int Step = 0; Step < 2400 && Bed.Outcome() == SleepOutcome::InProgress; ++Step)
        {
            if ((!Bed.IsLampOn() && Bed.Closet() >= .75) || (Bed.IsLampOn() && Bed.Closet() <= .45))
                Bed.ToggleLamp();
            Bed.Advance(.05);
        }
        Check(Bed.Outcome() == SleepOutcome::Asleep && Bed.Progress() == 1,
              "Active lamp management can win at the final-night pressure");
    }
    SprintHorror Glimpses;
    Glimpses.Advance(3, false);
    Glimpses.Advance(.85, true);
    Glimpses.Advance(2, false);
    Glimpses.Advance(.8, true);
    Check(Glimpses.Frame().Blackout == 0, "Rest drains sprint buildup before the next burst");
    Glimpses.Advance(.71, true);
    Check(Glimpses.Frame().Blackout == 1 && Glimpses.Frame().Figure == 0, "Fast movement begins with a black flash");
    Glimpses.Advance(.095, true);
    Check(Glimpses.Frame().Figure > .9 && Glimpses.Frame().Variant == 0 && !Glimpses.Frame().Right,
          "The first peripheral figure appears only inside the flash");
    Check(Glimpses.Frame(true).Blackout < .5 && Glimpses.Frame(true).Figure < .65,
          "Reduced visual intensity softens both parts of the glimpse");
    Glimpses.Advance(.19, true);
    Check(Glimpses.Frame().Blackout == 0 && Glimpses.Frame().Figure == 0, "The flash and figure clear in under a third of a second");
    Glimpses.Advance(1.5, true);
    Glimpses.Advance(.095, true);
    Check(Glimpses.Frame().Variant == 1 && Glimpses.Frame().Right, "Successive glimpses vary the figure and side");
    Glimpses.Interrupt();
    Glimpses.Advance(1.5, true);
    Glimpses.Advance(.095, true);
    Check(Glimpses.Frame().Variant == 2 && !Glimpses.Frame().Right, "Interrupting a flash preserves variety but clears buildup");
    Glimpses.Interrupt();
    Glimpses.Advance(1.49, true);
    Check(Glimpses.Frame().Blackout == 0, "A newly active phase starts without stored sprint buildup");
    return R;
}
} // namespace SA
