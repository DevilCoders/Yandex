#include "delay.h"
#include <util/datetime/base.h>
#include <util/random/random.h>
#include <util/string/vector.h>

void TSearchRequestDelay::Activate() const {
    if (!IsValid)
        return;
    if (!DelayProbability)
        return;
    if (DelayProbability == 100 || (RandomNumber<ui32>(100) + 1 <= (ui32)DelayProbability)) {
        Sleep(TDuration::MicroSeconds(Sleepus));
    }
}

TSearchRequestDelay::TSearchRequestDelay(const TString& paramsConf) {
    TVector<TString> params = SplitString(paramsConf, ".", 0, KEEP_EMPTY_TOKENS);
    if (params.size()) {
        IsValid = true;
        DelayTarget = params[0];
    }
    if (params.size() > 1) {
        IsValid = TryFromString(params[1], Sleepus) && (Sleepus > 0);
    }
    if (params.size() > 2) {
        IsValid = TryFromString(params[2], DelayProbability);
        DelayProbability = Max<i32>(0, Min<i32>(100, DelayProbability));
    }
}

