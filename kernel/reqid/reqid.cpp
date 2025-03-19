#include "reqid.h"

#include <library/cpp/charset/codepage.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/datetime/base.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/random/random.h>
#include <util/stream/str.h>
#include <util/string/ascii.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/system/hostname.h>

namespace {
    struct TReqIdHostSuffixHolder {
        inline TReqIdHostSuffixHolder()
            : HostSuffix(TStringBuf(HostName()).Before('.'))
        {
        }

        TString HostSuffix;
    };

    TString GetReqIdPrefixPattern() {
        // regex for <timestamp>-<hash>-<arbitrary_hostname_with_optional_suffix>

        constexpr TStringBuf timestampReg = "([[:digit:]]{16})";
        constexpr TStringBuf hashReg = "([[:digit:]]+)";
        constexpr TStringBuf hostnameAndSuffix = TStringBuf("[a-zA-Z0-9][a-zA-Z0-9_\\-\\.]+");

        return TString::Join(timestampReg, "-", hashReg, "-", hostnameAndSuffix);
    }
}

const TString& ReqIdHostSuffix() {
    return (Singleton<TReqIdHostSuffixHolder>())->HostSuffix;
}

TString ReqIdGenerate(const char* reqIdClass) {
    TStringStream id;
    id << TInstant::Now().GetValue() << '-' << RandomNumber<ui32>() << '-' << ReqIdHostSuffix();
    if (reqIdClass && *reqIdClass) {
        id << '-' << reqIdClass;
    }
    return id.Str();
}

void ReqIdParse(const TString& reqId, TInstant& reqIdTimestamp, TString& reqIdClass) {
    // ReqId format: <timestamp>-<hash>-<(cluster+machine(old_format)|arbitrary_hostname)>(-<reqid class>)?(-<pPAGE>)?(-REASK)?
    reqIdTimestamp = TInstant::MicroSeconds(0);
    reqIdClass.clear();
    TStringBuf reqIdBuf = reqId;
    // cut timestamp
    TStringBuf timestampBuf = reqIdBuf.NextTok('-');
    ui64 timestamp = 0;
    if (TryFromString(timestampBuf, timestamp)) {
        reqIdTimestamp = TInstant::MicroSeconds(timestamp);
    }
    // find reqid class: first all-uppercased chunk
    while (reqIdBuf) {
        TStringBuf chunk = reqIdBuf.NextTok('-');
        if (!chunk) {
            continue;
        }
        bool allUpper = true;
        for (size_t i = 0; i < chunk.size(); ++i) {
            if (!IsAsciiUpper(chunk[i])) {
                allUpper = false;
                break;
            }
        }
        if (allUpper && chunk != TStringBuf("REASK")) {
            reqIdClass = chunk;
            return;
        }
    }
}


bool ValidateReqId(TStringBuf reqId) {
    static const re2::RE2 precompiledPattern(GetReqIdPrefixPattern());

    return re2::RE2::FullMatch(re2::StringPiece(reqId.data(), reqId.length()), precompiledPattern);
}
