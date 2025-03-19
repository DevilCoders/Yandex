#pragma once

#include "counter.h"

#include <util/ysaveload.h>

#include <cmath>

namespace NFastUserFactors {

    class TDecayCounter: public ICounter {
    public:
        explicit TDecayCounter(IInputStream& in) {
            Load(in);
        }

        /**
         * @brief Exponential decay counter
         * @param decayParam - in decayParam days counter will be decreased in decayBase times
         * @param decayBase - multiplier for decay [default is 100]
         */
        explicit TDecayCounter(const float decayParam, const float decayBase = 1.0e2f) noexcept
            : DecayParam_(TDecayCounter::GetTrueDecay(decayParam, decayBase))
            , LastTs_(0)
            , Value_(0.0f)
        {
        }

        void Add(const time_t ts, const float value) noexcept override {
            if (ts > LastTs_) {
                Value_ = LastTs_ > 0
                    ? MoveWithTrueDecay(DecayParam_, Value_, LastTs_, ts) + value
                    : value;
                LastTs_ = ts;
            } else {
                Value_ += MoveWithTrueDecay(DecayParam_, value, ts, LastTs_);
            }
        }

        float Accumulate() const noexcept override {
            return Value_;
        }

        void Move(const time_t ts) noexcept override {
            Value_ = MoveWithTrueDecay(DecayParam_, Value_, LastTs_, ts);
            LastTs_ = ts;
        }

        ui64 LastTs() const noexcept override {
            return LastTs_;
        }

        void Clear() noexcept override {
            LastTs_ = 0;
            Value_ = 0.0f;
        }

        void Save(IOutputStream& out) const override {
            ::Save(&out, DecayParam_);
            ::Save(&out, LastTs_);
            ::Save(&out, Value_);
        }

        void Load(IInputStream& in) override {
            ::Load(&in, DecayParam_);
            ::Load(&in, LastTs_);
            ::Load(&in, Value_);
        }

        static float GetTrueDecay(const float decayParam, const float decayBase = 1.0e2f) noexcept {
            return decayParam > 0.0f ? std::log(decayBase > 1.f ? decayBase : 1.f) / (decayParam * 24 * 60 * 60) : 0.0f;
        }

        static float MoveWithTrueDecay(const float trueDecay, const float value, const time_t prevTime, const time_t newTime) {
            return value * std::exp(trueDecay * (prevTime - newTime));
        }

    private:
        float DecayParam_;
        time_t LastTs_;
        float Value_;
    };

    float MoveCounter(
        const float value,
        const ui64 srcTs,
        const ui64 dstTs,
        const float decayDays
    ) noexcept;

} // NFastUserFactors
