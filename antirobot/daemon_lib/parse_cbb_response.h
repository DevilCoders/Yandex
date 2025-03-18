#pragma once

#include "addr_list.h"

#include <antirobot/lib/addr.h>

#include <util/datetime/base.h>
#include <util/string/split.h>

namespace NAntiRobot {
    namespace NParseCbb {
        void ParseAddrWithExpire(const TStringBuf& line, TAddr& addrBegin, TAddr& addrEnd, TInstant& expire);
        void ParseAddrList(TAddrSet& addrSet, const TString& cbbAnswer);
        inline void ParseTextList(TVector<TString>& list, const TString& cbbAnswer) {
            Split(cbbAnswer, "\n", list);
        }
    }
}
