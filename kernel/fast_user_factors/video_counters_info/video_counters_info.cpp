#include "video_counters_info.h"

#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/string/split.h>

using namespace NFastUserFactors;

bool NFastUserFactors::TryParseVideoCounterName(const TStringBuf& name, std::pair<EVideoCounter, EVideoDecay>& counter) {
    for (const auto& decay : DECAY_VALUES) {
        TStringBuf prefix;
        if (name.BeforeSuffix("_" + ToString(decay.first), prefix)) {
            EVideoCounter videoCounter;
            if (TryFromString(prefix, videoCounter)) {
                counter.first = videoCounter;
                counter.second = decay.first;
                return true;
            }
        }
    }
    return false;
}

TString NFastUserFactors::UnderscoresToCamelCase(const TString& counter) {
    TVector<TString> tokens;
    StringSplitter(counter).Split('_').SkipEmpty().Collect(&tokens);

    for (TString& token : tokens) {
        token.to_upper(0, 1);
    }

    return JoinStrings(tokens, "");
}
