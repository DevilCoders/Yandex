#pragma once

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NRemorphParser {

class TTimeMonitor {
private:
    const TDuration& Limit;
    TString Doc;
    TSimpleTimer Timer;
public:
    TTimeMonitor(const TDuration& l, const TString& doc)
        : Limit(l)
        , Doc(doc)
    {
    }

    ~TTimeMonitor() {
        TDuration d = Timer.Get();
        if (d > Limit) {
            Cerr << Doc << ": takes too long processing time: " << d << Endl;
        }
    }
};

} // NRemorphParser
