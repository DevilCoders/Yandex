#include "querydata_traits.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <util/draft/datetime.h>

#include <util/string/printf.h>

namespace NQueryData {

    static_assert(Y_ARRAY_SIZE(TRIE_NAMES) == TV_COUNT, "expect Y_ARRAY_SIZE(TRIE_NAMES) == TV_COUNT");

    template <size_t N>
    static int GetIdFromName(TStringBuf s, const char* const (&names)[N], int deflt = -1) {
        for (int i = 0; i < (int)N; ++i) {
            if (s == names[i]) {
                return i;
            }
        }
        return deflt;
    }

    template <size_t N>
    static TStringBuf GetNameFromId(int id, const char* const (&names)[N], TStringBuf deflt = "INVALID") {
        if (id < 0 || id >= (int)N) {
            return deflt;
        }
        return names[id];
    }

    int GetTrieVariantIdFromName(TStringBuf s) {
        return GetIdFromName(s, TRIE_NAMES);
    }

    TStringBuf GetTrieVariantNameFromId(int t) {
        return GetNameFromId(t, TRIE_NAMES);
    }

    bool IsMobileSerpType(TStringBuf serpType) {
        return SERP_TYPE_MOBILE == serpType || SERP_TYPE_SMART == serpType || SERP_TYPE_TOUCH == serpType;
    }

    const TStringBufs& GetAllSerpTypes() {
        static const TStringBufs serpTypes{
            SERP_TYPE_TOUCH, SERP_TYPE_SMART, SERP_TYPE_MOBILE, SERP_TYPE_TABLET, SERP_TYPE_DESKTOP
        };
        return serpTypes;
    }

    struct TKeyTypeRegistry {
        typedef THashMap<int, TKeyTypeDescr> TId2Descr;
        typedef THashMap<TStringBuf, TKeyTypeDescr> TName2Descr;

        TKeyTypeDescrs Descrs;
        TId2Descr ById;
        TName2Descr ByName;

        TKeyTypeRegistry() {
            Descrs = {
                        (TKeyTypeDescr(KT_QUERY_EXACT, "none", "UserRequest", "user_request as is (no normalizations)")),
                        (TKeyTypeDescr(KT_QUERY_LOWERCASE, "lowercase", "UserRequest", "ToLower(user_request)")),
                        (TKeyTypeDescr(KT_QUERY_SIMPLE, "simple", "UserRequest", "FixSomeLetters(ToLower(AlphaNumOrSpace(user_request)))")),
                        (TKeyTypeDescr(KT_QUERY_STRONG, "strong", "UserRequest", "relev=norm")),
                        (TKeyTypeDescr(KT_QUERY_DOPPEL, "doppel", "UserRequest", "relev=dnorm")),
                        (TKeyTypeDescr(KT_QUERY_DOPPEL_TOKEN, "doppeltok", "UserRequest", "Any token from relev=dnorm")),
                        (TKeyTypeDescr(KT_QUERY_DOPPEL_PAIR, "doppelpair", "UserRequest", "Any pair of tokens from relev=dnorm")),
                        (TKeyTypeDescr(KT_DOCID, "docid", "Document",
                                       "DocHandle aka DocId of a document, rearr=docids, rearr=qd_urls, (ZABCDEF0123456789, ...)")),
                        (TKeyTypeDescr(KT_SNIPDOCID, "snipdocid", "Document",
                                       "DocHandle aka DocId of a top-30 document, rearr=snipdocids, rearr=qd_snipurls")),
                        (TKeyTypeDescr(KT_URL, "url", "Document",
                                       "Url of a document, rearr=qd_urls")),
                        (TKeyTypeDescr(KT_CATEG, "exacturl", "Document",
                                       "(Deprecated) owner (group categ in websearch) of a document, rearr=qd_url, (yandex.com.tr, ...)")),
                        (TKeyTypeDescr(KT_CATEG, "categ", "Document",
                                       "Owner (group categ in websearch) of a document, rearr=qd_categs, rearr=qd_urls, (yandex.com.tr, ...)")),
                        (TKeyTypeDescr(KT_SNIPCATEG, "snipcateg", "Document",
                                       "Owner (group categ in websearch) of a top-30 document, rearr=qd_snipcategs, rearr=qd_snipurls (yandex.com.tr, ...)")),
                        (TKeyTypeDescr(KT_CATEG_URL, "categurl", "Document",
                                       "Owner (group categ in websearch) and url of a document, used in antispam masks trie, rearr=qd_urls")),
                        (TKeyTypeDescr(KT_SNIPCATEG_URL, "snipcategurl", "Document",
                                       "Owner (group categ in websearch) and url of a document, used in antispam masks trie, rearr=qd_snipurls")),
                        (TKeyTypeDescr(KT_SNIPCATEG, "snipcateg", "Document",
                                       "Owner (group categ in websearch) of a top document, rearr=qd_snipcategs, rearr=qd_snipurls")),
                        (TKeyTypeDescr(KT_USER_ID, "yid", "User", "yandexuid, rearr=yandexuid")),
                        (TKeyTypeDescr(KT_USER_LOGIN, "ylogin", "User", "Login, rearr=login")),
                        (TKeyTypeDescr(KT_USER_LOGIN_HASH, "yloginhash", "User", "Login hash, rearr=loginhash")),
                        (TKeyTypeDescr(KT_USER_REGION, "region", "User",
                                       "User region or any superregion, rearr=all_regions, (0, 213, ...)")),
                        (TKeyTypeDescr(KT_USER_REGION_IPREG, "ipregregion", "User",
                                       "User region or any superregion, rearr=urcntr, (0, 225, ...)")),
                        (TKeyTypeDescr(KT_USER_IP_TYPE, "iptype", "User", "User ip type, rearr=gsmop (*, mobile_any, ...)")),
                        (TKeyTypeDescr(KT_SERP_TLD, "tld", "Yandex", "tld (*, ru, com.tr, ua ...)")),
                        (TKeyTypeDescr(KT_SERP_UIL, "uil", "Yandex", "uil (*, ru, tr, uk ...)")),
                        (TKeyTypeDescr(KT_SERP_DEVICE, "serptype", "Yandex", "Yandex serp type, rearr=device (*, desktop, tablet, mobile, smart, touch)")),
                        (TKeyTypeDescr(KT_STRUCTKEY, "structkey", "Trie",
                                       "Unspecified semantics, rearr=qd_struct_keys={namespace:{key1:null,key2:null}}")),
                        (TKeyTypeDescr(KT_BINARYKEY, "binarykey", "Trie",
                                       "Unspecified semantics, binary data. Expects base64 for subkey in indexer and raw data in request")),
                        (TKeyTypeDescr(FAKE_KT_STRUCTKEY_ANY, "structkeyany", "Trie(fake)", "")),
                        (TKeyTypeDescr(FAKE_KT_STRUCTKEY_ORDERED, "structkeyordered", "Trie(fake)", "")),
                        (TKeyTypeDescr(FAKE_KT_SOURCE_NAME, "sourcename", "Trie(fake)", ""))
                    };

            for (const auto& descr : Descrs) {
                ById[descr.KeyType] = descr;
                ByName[descr.Name] = descr;
            }
        }

        TKeyTypeDescr GetById(int id) const {
            TId2Descr::const_iterator it = ById.find(id);
            if (it == ById.end()) {
                return TKeyTypeDescr::Invalid();
            } else {
                return it->second;
            }
        }

        TKeyTypeDescr GetByName(TStringBuf name) const {
            TName2Descr::const_iterator it = ByName.find(name);
            if (it == ByName.end()) {
                return TKeyTypeDescr::Invalid();
            } else {
                return  it->second;
            }
        }
    };

    TKeyTypeDescr TKeyTypeDescr::Invalid() {
        return TKeyTypeDescr(KT_COUNT, "INVALID", "INVALID", "INVALID");
    }

    bool TKeyTypeDescr::IsValid() const {
        return KeyType < KT_COUNT && KeyType > FAKE_KT_COUNT;
    }

    TKeyTypeDescr KeyTypeDescrByName(TStringBuf s) {
        return Default<TKeyTypeRegistry>().GetByName(s);
    }

    TKeyTypeDescr KeyTypeDescrById(int id) {
        return Default<TKeyTypeRegistry>().GetById(id);
    }

    int GetNormalizationTypeFromName(TStringBuf s) {
        return KeyTypeDescrByName(s).KeyType;
    }

    TStringBuf GetNormalizationNameFromType(int t) {
        return KeyTypeDescrById(t).Name;
    }

    TString PrintNormNames(char sep) {
        TString res;
        const TKeyTypeDescrs& descrs = GetRealKeyTypeDescrs();
        for (const auto& descr : descrs) {
            if (res) {
                res.append(sep);
            }
            res.append(descr.Name);
        }
        return res;
    }

    TKeyTypeDescrs GetRealKeyTypeDescrs() {
        TKeyTypeDescrs res;
        const TKeyTypeDescrs& descrs = Default<TKeyTypeRegistry>().Descrs;
        for (const auto& descr : descrs) {
            if (!descr.IsFake()) {
                res.push_back(descr);
            }
        }
        return res;
    }

    bool IsPrioritizedNormalization(int norm) {
        switch (norm) {
        default : return false;
        case KT_USER_REGION:
        case KT_USER_IP_TYPE:
        case KT_USER_REGION_IPREG:
        case KT_SERP_TLD:
        case KT_SERP_UIL:
        case KT_SERP_DEVICE:
        case FAKE_KT_STRUCTKEY_ORDERED:
            return true;
        }
    }

    bool IsBinaryNormalization(int norm) {
        return KT_BINARYKEY == norm;
    }

    bool NormalizationNeedsExplicitKeyInResult(int norm) {
        switch (norm) {
        default : return IsPrioritizedNormalization(norm);
        case KT_STRUCTKEY:
        case KT_QUERY_DOPPEL_TOKEN:
        case KT_QUERY_DOPPEL_PAIR:
        case KT_DOCID:
        case KT_SNIPDOCID:
        case KT_CATEG:
        case KT_SNIPCATEG:
        case KT_CATEG_URL:
            return true;
        }
    }

    bool SkipNormalizationInScheme(int norm) {
        return IsBinaryNormalization(norm);
    }

    time_t GetTimestampFromVersion(ui64 v) {
        if (v > Max<ui32>()) {
            v /= 1000000;
        } else if (v > 1000000000 && v < 2000000000) {
            // it's ok, it's a timestamp
        } else if (v > 20000000 && v < 30000000) {
            NDatetime::TSimpleTM t;
            t.SetRealDate(v / 10000, (v % 10000) / 100, v % 100);
            v = (time_t)t;
        }

        return v;
    }

    ui64 GetTimestampMicrosecondsFromVersion(ui64 v) {
        if (v > 1'000'000'000'000'000ull && v < 4'000'000'000'000'000ull) {
            return v;
        } else {
            return GetTimestampFromVersion(v) * 1'000'000ull;
        }
    }

}
