#pragma once

#include "qd_types.h"

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NQueryData {

    enum EFactorType {
        FT_NONE = 0, FT_STRING, FT_INT, FT_FLOAT
    };

    const int FAKE_KT_STRUCTKEY_ANY = -1;
    const int FAKE_KT_STRUCTKEY_ORDERED = -2;
    const int FAKE_KT_SOURCE_NAME = -3;
    const int FAKE_KT_COUNT = -4;

    const char SERP_TYPE_SMART[] = "smart";
    const char SERP_TYPE_TOUCH[] = "touch";
    const char SERP_TYPE_TABLET[] = "tablet";
    const char SERP_TYPE_DESKTOP[] = "desktop";
    const char SERP_TYPE_MOBILE[] = "mobile"; // smart + touch = is_mobile

    bool IsMobileSerpType(TStringBuf serpType);

    const TStringBufs& GetAllSerpTypes();

    const char MOBILE_IP_ANY[] = "mobile_any";
    const char REGION_ANY[] = "0";
    const char GENERIC_ANY[] = "*";

    const char SC_OLD_QUERY_DATA[] = "OldQueryData";
    const char SC_RT_QUERY_DATA[] = "RTQueryData";

    const char SC_COMMON[] = "common";
    const char SC_VALUES[] = "values";
    const char SC_TIMESTAMP[] = "timestamp";

    struct TKeyTypeDescr {
        int KeyType = 0;
        TStringBuf Name;
        TStringBuf Category;
        TStringBuf Help;

        TKeyTypeDescr() = default;

        TKeyTypeDescr(int kt, TStringBuf name, TStringBuf categ, TStringBuf help)
            : KeyType(kt)
            , Name(name)
            , Category(categ)
            , Help(help)
        {}

        bool IsValid() const;

        bool IsFake() const {
            return KeyType < 0;
        }

        static TKeyTypeDescr Invalid();

        bool operator== (const TKeyTypeDescr& other) const {
            return KeyType == other.KeyType;
        }
    };

    using TKeyTypeDescrs = TVector<TKeyTypeDescr>;

    TKeyTypeDescr KeyTypeDescrById(int keytype);
    TKeyTypeDescr KeyTypeDescrByName(TStringBuf name);

    const char* const TRIE_NAMES[] = {
        "comptrie", "codectrie", "solartrie", "metatrie", "codedblobtrie", "urlmaskstrie"
    };

    TString PrintNormNames(char sep = '|');
    TKeyTypeDescrs GetRealKeyTypeDescrs();

    int GetNormalizationTypeFromName(TStringBuf s);
    TStringBuf GetNormalizationNameFromType(int t);

    int GetTrieVariantIdFromName(TStringBuf s);
    TStringBuf GetTrieVariantNameFromId(int t);

    bool IsPrioritizedNormalization(int norm);
    bool IsBinaryNormalization(int norm);

    bool NormalizationNeedsExplicitKeyInResult(int norm);
    bool SkipNormalizationInScheme(int norm);

    time_t GetTimestampFromVersion(ui64 ver);

    ui64 GetTimestampMicrosecondsFromVersion(ui64 ver);

    const char FACTOR_VALUE_TYPE_BINARY[] = "B";
    const char FACTOR_VALUE_TYPE_STRING_ESCAPED[] = "b";
    const char FACTOR_VALUE_TYPE_STRING[] = "s";
    const char FACTOR_VALUE_TYPE_INT[] = "i";
    const char FACTOR_VALUE_TYPE_FLOAT[] = "f";

}
