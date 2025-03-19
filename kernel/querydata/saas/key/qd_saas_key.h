#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <utility>

namespace NQueryDataSaaS {

    enum class EQDSaaSType : ui32 {
        None = 0,
        KV,
        Trie
    };

    const char QD_SAAS_RECORD_KEY_PREFIX[] = "QDSaaS:";

    // Значения енума используются в данных, новые значения добавлять строго в конец!
    enum ESaaSSubkeyType : i32 {
        SST_INVALID = 0,
        SST_IN_KEY_QUERY_STRONG = 1        /* "QueryStrong" */,
        SST_IN_KEY_QUERY_DOPPEL = 2        /* "QueryDoppel" */,
        SST_IN_KEY_USER_ID = 3             /* "UserId" */,
        SST_IN_KEY_USER_LOGIN_HASH = 4     /* "UserLoginHash" */,
        SST_IN_KEY_USER_REGION = 5         /* "UserRegion" */,
        SST_IN_KEY_STRUCT_KEY = 6          /* "StructKey" */,
        SST_IN_KEY_URL = 7                 /* "Url" */,
        SST_IN_KEY_OWNER = 8               /* "Owner" */,
        SST_IN_KEY_URL_MASK = 9            /* "UrlMask" */,
        SST_IN_KEY_ZDOCID = 10             /* "ZDocId" */,
        SST_IN_KEY_QUERY_DOPPEL_TOKEN = 11 /* "QueryDoppelToken" */,
        SST_IN_KEY_QUERY_DOPPEL_PAIR = 12  /* "QueryDoppelPair" */,
        SST_IN_KEY_URL_NO_CGI = 13         /* "UrlNoCgi" */,
        SST_IN_KEY_COUNT = 14,

        SST_IN_VALUE = 0x100, // ключи с ограниченным диапазоном значений, которые помещаются в значения
        SST_IN_VALUE_USER_REGION_IPREG     /* "UserRegionIpReg" */,
        SST_IN_VALUE_USER_IP_TYPE          /* "UserIpType" */,
        SST_IN_VALUE_SERP_TLD              /* "SerpTLD" */,
        SST_IN_VALUE_SERP_UIL              /* "SerpUIL" */,
        SST_IN_VALUE_SERP_DEVICE           /* "SerpDevice" */,
        SST_IN_VALUE_COUNT
    };

    using TSaaSKeyType = TVector<ESaaSSubkeyType>;


    class TSaaSSubkey {
    public:
        TSaaSSubkey() = default;

        TSaaSSubkey(ESaaSSubkeyType t, const TString& v)
            : Key(v)
            , Type(t)
        {}

        TSaaSSubkey(ESaaSSubkeyType t, TString&& v)
            : Key(std::move(v))
            , Type(t)
        {}

        const TString& GetKey() const {
            return Key;
        }

        ESaaSSubkeyType GetType() const {
            return Type;
        }

    private:
        TString Key;
        ESaaSSubkeyType Type = SST_INVALID;
    };

    using TSaaSKey = TVector<TSaaSSubkey>;


    const TVector<ESaaSSubkeyType>& GetAllInKeySubkeys();

    const TVector<ESaaSSubkeyType>& GetAllInValueSubkeys();

    ESaaSSubkeyType GetTrieKeyForSharding(const TSaaSKeyType& keyType);

    bool IsSplittable(ESaaSSubkeyType);

    bool IsSplittable(const TSaaSKeyType&);

    const TVector<ESaaSSubkeyType>& GetAllTrieSubkeysForSearch();

    TSaaSKeyType ReorderTrieKeyForSearch(const TSaaSKeyType& keyType);

    bool SubkeyTypeIsValidForKey(ESaaSSubkeyType);

    bool SubkeyTypeIsValidForValue(ESaaSSubkeyType);

    bool SubkeyTypeIsValid(ESaaSSubkeyType);

    bool SubkeyTypeAllowedInKeyAlone(ESaaSSubkeyType);

    bool KeyTypeIsValid(const TSaaSKeyType&);

    void ParseFromString(TSaaSKeyType&, TStringBuf);

    void WriteToString(TString&, const TSaaSKeyType&);
}

const TString& ToString(NQueryDataSaaS::ESaaSSubkeyType x);
const TString& ToString(NQueryDataSaaS::EQDSaaSType x);
