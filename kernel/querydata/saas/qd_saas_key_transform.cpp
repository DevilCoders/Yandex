#include "qd_saas_key_transform.h"

#include <kernel/hosts/owner/owner.h>
#include <kernel/urlid/url2docid.h>

#include <library/cpp/infected_masks/infected_masks.h>

#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/string/ascii.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/util.h>

namespace NQueryDataSaaS {

    enum EUrlError {
        UE_NONE = 0,
        UE_NO_SCHEME = 1,
        UE_UPPER_CASE_HOST = 2,
    };


    static ui32 SplitAndCheckUrl(TStringBuf& scheme, TStringBuf& host, TStringBuf& suffix, const TStringBuf url) {
        static const TStringBuf http = "http://";
        static str_spn asciiUpper{"A-Z", true};

        ui32 error = UE_NONE;

        const size_t schemeSize = GetSchemePrefixSize(url);
        scheme = url.Head(schemeSize);

        if (!schemeSize) {
            scheme = http;
            error |= UE_NO_SCHEME;
        }

        suffix = url.Tail(schemeSize);
        host = GetHost(suffix);
        suffix = suffix.Tail(host.size());

        if (asciiUpper.brk(host.begin(), host.end()) != host.end()) {
            error |= UE_UPPER_CASE_HOST;
        }

        return error;
    }


    TStringBuf NormalizeDocUrl(TStringBuf url, TString& buffer) {
        if (!url) {
            return url;
        }

        TStringBuf scheme, host, suffix;
        if (auto error = SplitAndCheckUrl(scheme, host, suffix, url)) {
            buffer.clear();
            buffer.append(scheme);
            buffer.append(host);
            if (error & UE_UPPER_CASE_HOST) {
                buffer.to_lower(scheme.size());
            }
            buffer.append(suffix);
            return buffer;
        } else {
            // no error
            return url;
        }
    }


    TString NormalizeDocUrl(const TString& url) {
        TString buf;
        return (url == NormalizeDocUrl(url, buf)) ? url : buf;
    }


    TStringBuf GetUrlMaskFromCategUrl(TStringBuf categUrl) {
        return categUrl.After(' ');
    }


    TStringBuf GetOwnerFromNormalizedDocUrl(TStringBuf url) {
        return TOwnerCanonizer::WithSerpCategOwners().GetUrlOwner(url);
    }


    void GetMasksFromNormalizedDocUrl(TVector<NInfectedMasks::TLiteMask>& result, TStringBuf url) {
        using namespace NInfectedMasks;
        const TStringBuf owner = GetOwnerFromNormalizedDocUrl(url);
        GenerateMasksFast(result, url, NInfectedMasks::C_YANDEX_GOOGLE_ORDER);
        // Это важная проверка, которая, например, не позволит сматчить yandex.com.tr на маску com.tr
        // Google order перечисляет маски начиная с самого длинного хоста,
        // поэтому все маски с хостом короче владельца находятся в хвосте вектора
        while (result && owner.size() > result.back().Host.size()) {
            result.pop_back();
        }
    }


    void GetMasksFromNormalizedDocUrl(TVector<TString>& res, TStringBuf url) {
        TVector<NInfectedMasks::TLiteMask> masks;
        GetMasksFromNormalizedDocUrl(masks, url);
        for (const auto& mask : masks) {
            mask.Render(res.emplace_back());
        }
    }


    TStringBuf GetZDocIdFromNormalizedUrl(TStringBuf url, TString& buffer) {
        return Url2DocId(url, buffer);
    }


    TString GetZDocIdFromNormalizedUrl(TStringBuf url) {
        TString buf;
        GetZDocIdFromNormalizedUrl(url, buf);
        return buf;
    }

    TStringBuf GetNoCgiFromNormalizedUrl(const TStringBuf url) {
        return url.Before('?').Before('#');
    }

    TString GetNoCgiFromNormalizedUrlStr(const TString& url) {
        return TString(GetNoCgiFromNormalizedUrl(url));
    }

    TStringBuf GetStructKeyFromPair(TStringBuf nSpace, TStringBuf key, TString& buffer) {
        return buffer.assign(nSpace).append('@').append(key);
    }


    TString GetStructKeyFromPair(TStringBuf nSpace, TStringBuf key) {
        TString buf;
        GetStructKeyFromPair(nSpace, key, buf);
        return buf;
    }


    bool ParseStructKey(TStringBuf& nSpace, TStringBuf& key, TStringBuf sk) {
        if (sk.TrySplit('@', nSpace, key) && nSpace && key) {
            return true;
        } else {
            nSpace.Clear();
            key.Clear();
            return false;
        }
    }


    ESaaSSubkeyType GetSaaSKeyType(NQueryData::EKeyType kt) {
        switch (kt) {
        default:
            return SST_INVALID;

        case NQueryData::KT_USER_REGION_IPREG:
            return SST_IN_VALUE_USER_REGION_IPREG;

        case NQueryData::KT_USER_IP_TYPE:
            return SST_IN_VALUE_USER_IP_TYPE;

        case NQueryData::KT_SERP_TLD:
            return SST_IN_VALUE_SERP_TLD;

        case NQueryData::KT_SERP_UIL:
            return SST_IN_VALUE_SERP_UIL;

        case NQueryData::KT_SERP_DEVICE:
            return SST_IN_VALUE_SERP_DEVICE;

        case NQueryData::KT_USER_REGION:
            return SST_IN_KEY_USER_REGION;

        case NQueryData::KT_STRUCTKEY:
            return SST_IN_KEY_STRUCT_KEY;

        case NQueryData::KT_QUERY_STRONG:
            return SST_IN_KEY_QUERY_STRONG;

        case NQueryData::KT_QUERY_DOPPEL:
            return SST_IN_KEY_QUERY_DOPPEL;

        case NQueryData::KT_QUERY_DOPPEL_TOKEN:
            return SST_IN_KEY_QUERY_DOPPEL_TOKEN;

        case NQueryData::KT_QUERY_DOPPEL_PAIR:
            return SST_IN_KEY_QUERY_DOPPEL_PAIR;

        case NQueryData::KT_USER_ID:
            return SST_IN_KEY_USER_ID;

        case NQueryData::KT_USER_LOGIN_HASH:
            return SST_IN_KEY_USER_LOGIN_HASH;

        case NQueryData::KT_DOCID:
        case NQueryData::KT_SNIPDOCID:
            return SST_IN_KEY_ZDOCID;

        case NQueryData::KT_CATEG:
        case NQueryData::KT_SNIPCATEG:
            return SST_IN_KEY_OWNER;

        case NQueryData::KT_CATEG_URL:
        case NQueryData::KT_SNIPCATEG_URL:
            return SST_IN_KEY_URL_MASK;

        case NQueryData::KT_URL:
            return SST_IN_KEY_URL;

        case NQueryData::KT_URL_NO_CGI:
            return SST_IN_KEY_URL_NO_CGI;
        }
    }


    NQueryData::EKeyType GetQueryDataKeyType(ESaaSSubkeyType kt) {
        switch (kt) {
        default:
            return NQueryData::KT_COUNT;
        case SST_IN_KEY_QUERY_STRONG:
            return NQueryData::KT_QUERY_STRONG;

        case SST_IN_KEY_QUERY_DOPPEL:
            return NQueryData::KT_QUERY_DOPPEL;

        case SST_IN_KEY_QUERY_DOPPEL_TOKEN:
            return NQueryData::KT_QUERY_DOPPEL_TOKEN;

        case SST_IN_KEY_QUERY_DOPPEL_PAIR:
            return NQueryData::KT_QUERY_DOPPEL_PAIR;

        case SST_IN_KEY_USER_ID:
            return NQueryData::KT_USER_ID;

        case SST_IN_KEY_USER_LOGIN_HASH:
            return NQueryData::KT_USER_LOGIN_HASH;

        case SST_IN_KEY_USER_REGION:
            return NQueryData::KT_USER_REGION;

        case SST_IN_KEY_STRUCT_KEY:
            return NQueryData::KT_STRUCTKEY;

        case SST_IN_KEY_URL:
            return NQueryData::KT_URL;

        case SST_IN_KEY_URL_NO_CGI:
            return NQueryData::KT_URL_NO_CGI;

        case SST_IN_KEY_OWNER:
            return NQueryData::KT_CATEG;

        case SST_IN_KEY_URL_MASK:
            return NQueryData::KT_CATEG_URL;

        case SST_IN_KEY_ZDOCID:
            return NQueryData::KT_DOCID;

        case SST_IN_VALUE_USER_REGION_IPREG:
            return NQueryData::KT_USER_REGION_IPREG;

        case SST_IN_VALUE_USER_IP_TYPE:
            return NQueryData::KT_USER_IP_TYPE;

        case SST_IN_VALUE_SERP_TLD:
            return NQueryData::KT_SERP_TLD;

        case SST_IN_VALUE_SERP_UIL:
            return NQueryData::KT_SERP_UIL;

        case SST_IN_VALUE_SERP_DEVICE:
            return NQueryData::KT_SERP_DEVICE;
        }
    }


    TSaaSKeyType GetSaaSKeyType(const NQueryData::TKeyTypeVec& qdKeyType) {
        TSaaSKeyType saaSKeyType;
        for (auto kt : qdKeyType) {
            auto sst = GetSaaSKeyType(kt);
            if (SubkeyTypeIsValidForKey(sst)) {
                saaSKeyType.emplace_back(sst);
            }
        }
        return saaSKeyType;
    }

}
