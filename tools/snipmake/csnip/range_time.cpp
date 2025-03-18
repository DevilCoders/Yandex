#include "range_time.h"

#include <util/generic/yexception.h>

namespace NSnippets {

    bool TRangeTime::Fits(const TDuration& t) const {
        return From <= t && t <= To;
    }

    void TRangeTime::Parse(const TString& s) {
        if (!s.size()) {
            ythrow yexception() << "Empty time range supplied";
        }
        if (s[0] == '+' || s[0] == '-') {
            TDuration t = TDuration::Parse(s.data() + 1);
            if (s[0] == '+') {
                From = t;
            } else {
                To = t;
            }
        } else {
            const size_t d = s.find("..");
            if (d == TString::npos) {
                ythrow yexception() << "Unrecognized time range: " << s;
            }
            From = TDuration::Parse(TString(s.data(), s.data() + d));
            To = TDuration::Parse(TString(s.data() + d + 2, s.data() + s.size()));
        }
    }

} //namespace NSnippets
