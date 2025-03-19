#pragma once
#include <library/cpp/http/misc/httpreqdata.h>
#include <util/generic/string.h>
#include <util/string/type.h>

class TSearchRequestDelay {
private:
    TString DelayTarget;
    bool IsValid = false;
    i32 Sleepus = 10000000;
    i32 DelayProbability = 100;
public:

    TSearchRequestDelay(const TString& paramsConf);

    bool Valid() const {
        return IsValid;
    }

    const TString& GetDelayTarget() const {
        return DelayTarget;
    }

    void Activate() const;

    static void Process(const TCgiParameters& cgi, const TString& target) {
        if (IsTrue(cgi.Get("DF")) || (cgi.Has("info")))
            return;
        for (size_t i = 0; i < cgi.NumOfValues("delay"); ++i) {
            const TString& delayParams = cgi.Get("delay", i);
            if (!!delayParams) {
                TSearchRequestDelay srd(delayParams);
                if (srd.GetDelayTarget() == target) {
                    srd.Activate();
                }
            }
        }
    }

};
