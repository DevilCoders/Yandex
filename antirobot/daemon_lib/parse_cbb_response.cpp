#include "parse_cbb_response.h"

#include "eventlog_err.h"

#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace NAntiRobot {

namespace NParseCbb {

void ParseAddrWithExpire(const TStringBuf& line, TAddr& addrBegin, TAddr& addrEnd, TInstant& expire) {
    TStringBuf addrStrBegin;
    TStringBuf addrStrEnd;
    TStringBuf expireStr;

    try {
        Split(line, ';', addrStrBegin, addrStrEnd, expireStr);
    } catch (yexception&) {
        ythrow yexception() << "Couldn't parse '" << line << "'";
    }

    addrBegin = TAddr(StripString(addrStrBegin));
    addrEnd = TAddr(StripString(addrStrEnd));
    const size_t expireTime = FromStringWithDefault<size_t>(StripString(expireStr), 0);
    expire = expireTime != 0 ? TInstant::Seconds(expireTime) : TInstant::Max();
}

void ParseAddrList(TAddrSet& addrSet, const TString& cbbAnswer) {
    TStringInput cbbResStream(cbbAnswer);
    TString line;
    while (cbbResStream.ReadLine(line)) {
        if (!line.empty()) {
            try {
                TAddr addrBegin;
                TAddr addrEnd;
                TInstant expire;
                ParseAddrWithExpire(line, addrBegin, addrEnd, expire);

                if (!addrSet.Add(TIpInterval(addrBegin, addrEnd), expire)) {
                    EVLOG_MSG << "Range " << addrBegin << ":" << addrEnd << " has not been add";
                }
            } catch(...) {
                EVLOG_MSG << CurrentExceptionMessage();
            }
        }
    }
}

}
}
