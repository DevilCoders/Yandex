#pragma once

#include <library/cpp/regex/pire/regexp.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/file.h>

namespace NAntiRobot {
    class TBadUserAgents {
    public:
        void Load(IInputStream& in, bool isCaseInsensitive = true);

        inline bool IsUserAgentBad(const TStringBuf& ua) const noexcept {
            if (ua.size() <= 5)
                return true;

            for (TFsmVector::const_iterator toFsm = Fsms.begin(); toFsm != Fsms.end(); ++toFsm) {
                if (NPire::Runner(toFsm->GetScanner()).Begin().Run(ua.data(), ua.size()).End()) {
                    return true;
                }
            }

            return false;
        }

    private:
        typedef TVector<NRegExp::TFsm> TFsmVector;
        TFsmVector Fsms;
    };
}
