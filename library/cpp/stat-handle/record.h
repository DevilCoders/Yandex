#pragma once

#include <util/datetime/base.h>

#include <array>

namespace NStat {
    enum class EResult {
        Total,
        Ok,
        No,
        Error,
        Cached,
        Max
    };

    struct TDistribution {
        // counts all the requests that were processed by current rule
        // using equal or more than DurationAtLeast milliseconds
        struct TCounter {
            std::array<size_t, static_cast<size_t>(EResult::Max)> Count = {};
            size_t operator[](EResult r) const {
                return Count[static_cast<size_t>(r)];
            }
            size_t& operator[](EResult r) {
                return Count[static_cast<size_t>(r)];
            }
        };

        static const size_t NUM_COUNTERS = 6;
        static const std::array<size_t, NUM_COUNTERS> STEPS; // step durations in ms

        static TDuration DurationStep(size_t i) {
            return TDuration::MilliSeconds(STEPS[i]);
        }

        std::array<TCounter, NUM_COUNTERS> Counters;

    public:
        TDistribution& operator+=(const TDistribution& distr);

        void Add(const TDuration& duration, EResult t);

        // Counters[0] represents total distribution, AtLeast = 0
        const TCounter& Total() const {
            return Counters[0];
        }
    };

    struct TTimeInfo {
        TInstant LastStarted;
        TInstant LastFinished;
        TDuration Duration;
        TDistribution Distribution;

        TTimeInfo& operator+=(const TTimeInfo& info) {
            Duration += info.Duration;
            Distribution += info.Distribution;

            return *this;
        }

        void StartedNow() {
            LastStarted = Now();
        }

        void FinishedNow(EResult t = EResult::Max) {
            LastFinished = Now();
            if (Y_LIKELY(LastStarted != TInstant() && LastStarted < LastFinished)) {
                TDuration currentDuration = LastFinished - LastStarted;
                Duration += currentDuration;
                Distribution.Add(currentDuration, t);
            }
        }

        // counts for total distribution
        size_t Count(EResult t) const {
            Y_ASSERT(t < EResult::Max);
            return Distribution.Total()[t];
        }

        TDuration AvgDuration() const {
            return Count(EResult::Total) > 0 ? Duration / Count(EResult::Total) : TDuration();
        }
    };

    class TStatProto; // protobuf serialization

    struct TRecord {
        TTimeInfo Timing;   // runtime statisics
        TTimeInfo InitTime; // duration of initialization
        size_t Memory = 0;  // memory used on initialization, counted if TraceMemUsage == true

        TRecord& operator+=(const TRecord& r) {
            Timing += r.Timing;
            InitTime += r.InitTime;
            Memory += r.Memory;
            return *this;
        }

        bool HasRuntime() const {
            return Timing.Count(EResult::Total) > 0;
        }

        bool HasInit() const {
            return InitTime.Count(EResult::Total) > 0;
        }

        bool IsPrintable(bool init, bool runtime) const {
            return (init && HasInit()) || (runtime && HasRuntime());
        }

        TString Print(const TStringBuf& name, bool init, bool runtime, const TRecord* total = nullptr) const;
        size_t SortingKey(bool init, bool runtime) const;

        void AddToProto(TStatProto& proto, const TString& name) const;

    private:
        TString PrintInit(const TRecord& total) const;
        TString PrintRuntime(const TRecord& total) const;
    };

    TString HumanReadablePercent(float f, float s);

}
