#include "image_counters_info.h"
#include <kernel/fast_user_factors/counters/decay_counter.h>

#include <util/generic/ymath.h>
#include <util/string/cast.h>
#include <util/string/vector.h>

bool NFastUserFactors::TryParseImageCounterName(const TStringBuf& name, std::pair<EImageCounter, EImageDecay>& counter) {
    for (const auto& decay : IMAGE_DECAY_VALUES) {
        TStringBuf prefix;
        if (name.BeforeSuffix("_" + ToString(decay.first), prefix)) {
            EImageCounter imageCounter;
            if (TryFromString(prefix, imageCounter)) {
                counter.first = imageCounter;
                counter.second = decay.first;
                return true;
            }
        }
    }
    return false;
}

void NFastUserFactors::ParseCountersFromRtmr(const TCountersProto& counters, const ui64& queryTimestamp, THashMap<TImageCounter, float>& counterValues) {
    THashMap<TImageCounter, TDecayCounter> decayCounters;

    const ui64 countersTimestamp = counters.GetTimestamp();
    for (const auto& counterValue : counters.GetCounters()) {
        TImageCounter counter;
        if (TryParseImageCounterName(counterValue.GetName(), counter)) {
            if (auto* counterPtr = decayCounters.FindPtr(counter)) {
                counterPtr->Add(countersTimestamp, counterValue.GetValue());
            } else {
                TDecayCounter decayCounter(IMAGE_DECAY_VALUES.at(counter.second));
                decayCounter.Add(countersTimestamp, counterValue.GetValue());
                decayCounters.insert(std::make_pair(counter, decayCounter));
            }
        }
    }

    for (auto& counter : decayCounters) {
        counter.second.Move(queryTimestamp);
        counterValues.insert({counter.first, counter.second.Accumulate()});
    }
}

float NFastUserFactors::GetCounterValue(const THashMap<TImageCounter, float>& counterValues, const TImageCounter& counter) {
    if (auto* counterValue = counterValues.FindPtr(counter)) {
        return *counterValue;
    } else {
        return 0.0;
    }
}

float NFastUserFactors::GetCounterRatio(const float numerator, const float denominator, const float maxValue) {
    if (Abs(numerator) < std::numeric_limits<float>::epsilon()) {
        return 0.0;
    }
    if (Abs(denominator) < std::numeric_limits<float>::epsilon()) {
        return maxValue;
    }

    return ClampVal<float>(numerator / denominator, 0.0, maxValue);
}
