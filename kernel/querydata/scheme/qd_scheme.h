#pragma once

#include <library/cpp/scheme/scheme.h>

namespace NQueryData {

    class TQueryData;
    class TSourceFactors;

    // legacy conversion (version 1)
    void QueryData2Scheme(NSc::TValue&, const TQueryData&, bool realtime = false);
    NSc::TValue QueryData2Scheme(const TQueryData&, bool realtime = false);

    // version 2 conversion
    void QueryData2SchemeV2(NSc::TValue&, const TQueryData&);
    NSc::TValue QueryData2SchemeV2(const TQueryData&);

    // version 3 conversion
    void QueryData2SchemeV3(NSc::TValue&, const TQueryData&);
    NSc::TValue QueryData2SchemeV3(const TQueryData&);

    // same for verson 1 and 2
    void DoFillValue(NSc::TValue&, const TSourceFactors&, bool includebinarydata = false);

    extern const TStringBuf SC_2_TIMESTAMP;
    extern const TStringBuf SC_2_NAMESPACE;
    extern const TStringBuf SC_2_TRIE_NAME;
    extern const TStringBuf SC_2_HOST_NAME;
    extern const TStringBuf SC_2_SHARD_NUMBER;
    extern const TStringBuf SC_2_SHARDS_TOTAL;
    extern const TStringBuf SC_2_IS_REALTIME;
    extern const TStringBuf SC_2_IS_COMMON;
    extern const TStringBuf SC_2_KEY;
    extern const TStringBuf SC_2_KEY_TYPE;
    extern const TStringBuf SC_2_VALUE;

    extern const TStringBuf SC_3_META;
    extern const TStringBuf SC_3_DATA;

}
