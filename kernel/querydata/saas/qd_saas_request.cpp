#include "qd_saas_request.h"
#include "qd_saas_key_transform.h"

#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/cgi/qd_request.h>
#include <kernel/querydata/idl/querydata_common.pb.h>
#include <kernel/querydata/request/qd_subkeys_util.h>
#include <kernel/querydata/saas/idl/saas_req_props.pb.h>

#include <kernel/hosts/owner/owner.h>
#include <kernel/saas_trie/idl/saas_trie.pb.h>
#include <kernel/saas_trie/idl/trie_key.h>

#include <library/cpp/infected_masks/infected_masks.h>

#include <util/string/split.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>

#include <kernel/querydata/cgi/qd_cgi_utils.h>

#include <util/ysaveload.h>

template <>
class TSerializer<TStringBuf> {
public:
    static inline void Save(IOutputStream* rh, const TStringBuf& s) {
        size_t length = s.size();
        ::SaveSize(rh, length);
        ::SavePodArray(rh, s.data(), length);
    }

    template <class TStorage>
    static inline void Load(IInputStream* rh, TStringBuf& s, TStorage& pool) {
        const size_t len = LoadSize(rh);

        char* res = AllocateFromPool(pool, len + 1);
        ::LoadPodArray(rh, res, len);
        res[len] = 0;
        s = TStringBuf{res, len};
    }
};

namespace NQueryDataSaaS {

    TSaaSRequestRec::TSaaSRequestRec()
    {
        SetUserRegionHierarchy({NQueryData::REGION_ANY});
        SetUserIpRegionHierarchy({NQueryData::REGION_ANY});

        SetUserIpOperatorType(NQueryData::GENERIC_ANY);
        SetSerpTopLevelDomain(NQueryData::GENERIC_ANY);
        SetSerpDeviceType(NQueryData::GENERIC_ANY);
        SetSerpUILang(NQueryData::GENERIC_ANY);
    }

    namespace {
        void DoFillSubkeyInferiority(THashMap<std::pair<int, TString>, ui32>& infer, const TVector<TString>& hier, int kt) {
            for (size_t i = 0, sz = hier.size(); i < sz; ++i) {
                infer[std::make_pair(kt, hier[i])] = (ui32) i;
            }
        }
    }

#define QD_SAAS_GENERATE_GET_SET(name, sst) \
    const TString& TSaaSRequestRec::Get##name() const { \
        return name; \
    } \
    void TSaaSRequestRec::Set##name(const TString& value) { \
        DoClearGeneratedKVSubkeys(sst); \
        name = value; \
    }

#define QD_SAAS_GENERATE_GET_SET_HIER(name, kt, sst) \
    const TVector<TString>& TSaaSRequestRec::Get##name##Hierarchy() const { \
        return name##Hierarchy; \
    } \
    void TSaaSRequestRec::Set##name(const TString& value) { \
        DoClearGeneratedKVSubkeys(sst); \
        NQueryData::AssignNotEmpty(name##Hierarchy, value); \
        NQueryData::EnsureHierarchy(name##Hierarchy, NQueryData::GENERIC_ANY); \
        DoFillSubkeyInferiority(ReqProps.GetSubkeyInferiorityMutable(), name##Hierarchy, kt); \
    }

#define QD_SAAS_GENERATE_GET_SET_HIER_REG(name, kt, sst) \
    const TVector<TString>& TSaaSRequestRec::Get##name##Hierarchy() const { \
        return name##Hierarchy; \
    } \
    void TSaaSRequestRec::Set##name##Hierarchy(const TVector<TString>& value) { \
        DoClearGeneratedKVSubkeys(sst); \
        NQueryData::AssignNotEmpty(name##Hierarchy, value); \
        NQueryData::EnsureHierarchy(name##Hierarchy, NQueryData::REGION_ANY); \
        DoFillSubkeyInferiority(ReqProps.GetSubkeyInferiorityMutable(), name##Hierarchy, kt); \
    }

#define QD_SAAS_GENERATE_GET_SET_HIER_SPEC(name, generator, kt, sst) \
    const TVector<TString>& TSaaSRequestRec::Get##name##Hierarchy() const { \
        return name##Hierarchy; \
    } \
    void TSaaSRequestRec::Set##name(const TString& value) { \
        DoClearGeneratedKVSubkeys(sst); \
        generator(name##Hierarchy, value); \
        DoFillSubkeyInferiority(ReqProps.GetSubkeyInferiorityMutable(), name##Hierarchy, kt); \
    }

    QD_SAAS_GENERATE_GET_SET(QueryStrong, SST_IN_KEY_QUERY_STRONG)

    QD_SAAS_GENERATE_GET_SET(UserId, SST_IN_KEY_USER_ID)

    QD_SAAS_GENERATE_GET_SET(UserLoginHash, SST_IN_KEY_USER_LOGIN_HASH)

    QD_SAAS_GENERATE_GET_SET_HIER(SerpTopLevelDomain, NQueryData::KT_SERP_TLD, SST_INVALID)

    QD_SAAS_GENERATE_GET_SET_HIER(SerpUILang, NQueryData::KT_SERP_UIL, SST_INVALID)

    QD_SAAS_GENERATE_GET_SET_HIER_REG(UserRegion, NQueryData::KT_USER_REGION, SST_IN_KEY_USER_REGION)

    QD_SAAS_GENERATE_GET_SET_HIER_REG(UserIpRegion, NQueryData::KT_USER_REGION_IPREG, SST_IN_VALUE_USER_REGION_IPREG)

    QD_SAAS_GENERATE_GET_SET_HIER_SPEC(UserIpOperatorType, NQueryData::FillUserIpOperatorTypeHierarchy,
                                       NQueryData::KT_USER_IP_TYPE, SST_IN_VALUE_USER_IP_TYPE)

    QD_SAAS_GENERATE_GET_SET_HIER_SPEC(SerpDeviceType, NQueryData::FillSerpDeviceTypeHierarchy,
                                       NQueryData::KT_SERP_DEVICE, SST_IN_VALUE_SERP_DEVICE)

    const TString& TSaaSRequestRec::GetQueryDoppel() const {
        return QueryDoppel;
    }

    void TSaaSRequestRec::SetQueryDoppel(const TString& value) {
        DoClearGeneratedKVSubkeys(SST_IN_KEY_QUERY_DOPPEL);
        DoClearGeneratedKVSubkeys(SST_IN_KEY_QUERY_DOPPEL_TOKEN);
        DoClearGeneratedKVSubkeys(SST_IN_KEY_QUERY_DOPPEL_PAIR);
        PreparedQueryDoppelTokenVec = false;
        PreparedQueryDoppelPairVec = false;
        QueryDoppel = value;
    }

    const THashSet<std::pair<TString, TString>>& TSaaSRequestRec::GetStructKeys() const {
        return StructKeys;
    }

    const THashMap<TString, TVector<TString>>& TSaaSRequestRec::GetStructKeyHierarchy() const {
        return StructKeyHierarchy;
    }

    void TSaaSRequestRec::DoInsertStructKey(TStringBuf nSpace, TStringBuf key) {
        StructKeysVec.emplace_back(Pool.AppendString(GetStructKeyFromPair(nSpace, key, StructKeyBuf)));
        if (IsGeneratedKVSubkeys(SST_IN_KEY_STRUCT_KEY)) {
            DoGenerateKVSubkey(SST_IN_KEY_STRUCT_KEY, StructKeysVec.back());
        }
    }

    void TSaaSRequestRec::AddStructKey(const TString& nSpace, const TString& key) {
        if (nSpace && key) {
            auto res = StructKeys.insert(std::make_pair(nSpace, key));

            if (res.second && PreparedStructKeys) {
                DoInsertStructKey(nSpace, key);
            }
        }
    }

    void TSaaSRequestRec::AddStructKeyHierarchy(const TString& nSpace, const TVector<TString>& smallToBig) {
        DoClearGeneratedKVSubkeys(SST_IN_KEY_STRUCT_KEY);
        StructKeysVec.clear();
        PreparedStructKeys = false;

        if (nSpace) {
            auto& hier = StructKeyHierarchy[nSpace];
            NQueryData::AssignNotEmpty(hier, smallToBig);
            for (size_t i = 0, sz = hier.size(); i < sz; ++i) {
                if (hier[i]) {
                    ReqProps.GetStructKeyInferiorityMutable()[std::make_pair(nSpace, hier[i])] = (ui32)i;
                }
            }
        }
    }

    const TStrBufPairVec& TSaaSRequestRec::GetUrlsVec() const {
        return UrlsVec;
    }

    const TStrBufVec& TSaaSRequestRec::GetUrlsNoCgiVec() const {
        return UrlsNoCgiVec;
    }

    const TStrBufPairVec& TSaaSRequestRec::GetZDocIdsVec() const {
        return ZDocIdsVec;
    }

    const TStrBufVec& TSaaSRequestRec::GetOwnersVec() const {
        return OwnersVec;
    }

    void TSaaSRequestRec::DoInsertOwner(TStringBuf owner, bool save) {
        TStrBufSet::insert_ctx ins;
        TStrBufSet::iterator it = OwnersSet.find(owner, ins);
        if (it == OwnersSet.end()) {
            if (save) {
                owner = Pool.AppendString(owner);
            }
            OwnersVec.emplace_back(owner);
            OwnersSet.insert_direct(owner, ins);

            if (IsGeneratedKVSubkeys(SST_IN_KEY_OWNER)) {
                DoGenerateKVSubkey(SST_IN_KEY_OWNER, owner);
            }
        }
    }

    TStrBufPair TSaaSRequestRec::DoInsertPair(ESaaSSubkeyType sst, TStrBufPairVec& vec, TStrBufMap& map, TStringBuf item, TStringBuf origItem, bool save)
    {
        if (save) {
            item = Pool.AppendString(item);
            if (origItem != item) {
                origItem = Pool.AppendString(origItem);
            } else {
                origItem = item;
            }
        }

        vec.emplace_back(item, origItem);

        if (IsGeneratedKVSubkeys(sst)) {
            DoGenerateKVSubkey(sst, item);
        }

        if (origItem != item) {
            map[item] = origItem;
        }

        return std::make_pair(item, origItem);
    }

    const TStrBufVec& TSaaSRequestRec::GetUrlMasksVec() const {
        return UrlMasksVec;
    };

    void TSaaSRequestRec::AddUrl(TStringBuf url) {
        if (auto normUrl = NormalizeDocUrl(url, UrlBuf)) {
            std::tie(normUrl, url) = DoInsertPair(SST_IN_KEY_URL, UrlsVec, ReqProps.GetUrlsMapMutable(), normUrl, url, true);

            if (PreparedOwners) {
                DoInsertOwner(GetOwnerFromNormalizedDocUrl(normUrl), false);
            }

            if (PreparedUrlMasks) {
                DoInsertUrlMasksFromSavedUrl(normUrl, url);
            }

            if (PreparedUrlsNoCgi) {
                DoInsertUrlNoCgiFromUrl(normUrl, url);
            }

            if (PreparedZDocIds) {
                DoInsertZDocIdsFromUrl(normUrl, url);
            }
        }
    }

    void TSaaSRequestRec::AddOwner(TStringBuf owner) {
        if (owner) {
            DoInsertOwner(owner, true);
        }
    }

    void TSaaSRequestRec::AddZDocId(TStringBuf docId) {
        if (docId) {
            DoInsertPair(SST_IN_KEY_ZDOCID, ZDocIdsVec, ReqProps.GetZDocIdsMapMutable(), docId, docId, true);
        }
    }

    void TSaaSRequestRec::AddFilterNamespace(const TString& filterNamespace) {
        if (filterNamespace) {
            FilterNamespacesSet.insert(filterNamespace);
        }
    }

    bool TSaaSRequestRec::HasStructKey(const TString& nSpace, const TString& key) const {
        const auto& nSk = std::make_pair(nSpace, key);
        return StructKeys.contains(nSk) || ReqProps.GetStructKeyInferiority().contains(nSk);
    }

    void TSaaSRequestRec::FillFromQueryData(const NQueryData::TRequestRec& rec) {
        SetQueryStrong(rec.StrongNorm);
        SetQueryDoppel(rec.DoppelNorm);
        SetUserId(rec.UserId);
        SetUserLoginHash(rec.UserLoginHash);

        ReqProps.GetSubkeyInferiorityMutable().clear();

        SetUserRegionHierarchy(rec.UserRegions);
        SetUserIpRegionHierarchy(rec.GetUserRegionsIpReg());
        SetUserIpOperatorType(rec.GetUserIpMobileOp());
        SetSerpTopLevelDomain(rec.YandexTLD);
        SetSerpUILang(rec.UILang);
        SetSerpDeviceType(rec.SerpType);

        StructKeys.clear();
        StructKeyHierarchy.clear();
        ReqProps.GetStructKeyInferiorityMutable().clear();

        for (const auto& sk : rec.StructKeys.GetDict()) {
            const TString nSpace = TString{sk.first};
            if (sk.second.IsDict()) {
                for (const auto& k : sk.second.GetDict()) {
                    AddStructKey(nSpace, TString{k.first});
                }
            } else if (sk.second.IsArray()) {
                AddStructKeyHierarchy(nSpace, TVector<TString>{sk.second.GetArray().begin(), sk.second.GetArray().end()});
            }
        }

        UrlsVec.clear();
        OwnersVec.clear();
        OwnersSet.clear();
        ZDocIdsVec.clear();
        UrlMasksVec.clear();
        UrlsNoCgiVec.clear();
        ReqProps.GetUrlMasksMapMutable().clear();
        ReqProps.GetUrlsNoCgiMapMutable().clear();
        FilterNamespacesSet.clear();
        ReqProps.SetIsRecommendationRequest(rec.IsRecommendationsRequest);

        for (const auto& url : rec.DocItems.Urls()) {
            AddUrl(url.first);
        }

        for (const auto& url : rec.DocItems.SnipUrls()) {
            AddUrl(url.first);
        }

        for (const auto& categ : rec.DocItems.Categs()) {
            AddOwner(categ);
        }

        for (const auto& categ : rec.DocItems.SnipCategs()) {
            AddOwner(categ);
        }

        for (const auto& docId : rec.DocItems.DocIds()) {
            AddZDocId(docId);
        }

        for (const auto& docId : rec.DocItems.SnipDocIds()) {
            AddZDocId(docId);
        }

        FilterNamespacesSet = rec.FilterNamespaces;
    }

    TString TSaaSRequestRec::GenerateCgiFilterParameters() const {
        TStringStream params;
        for (TString ns : FilterNamespacesSet) {
            CGIEscape(ns);
            params << "&gta=QDSaaS%3A" << ns;
        }
        return params.Str();
    }

    void TSaaSRequestRec::DoInsertZDocIdsFromUrl(TStringBuf normUrl, TStringBuf origUrl) {
        TStringBuf zDocId = GetZDocIdFromNormalizedUrl(normUrl, ZDocIdBuf0);
        TStringBuf origZDocId = origUrl == normUrl ? zDocId : GetZDocIdFromNormalizedUrl(origUrl, ZDocIdBuf1);
        DoInsertPair(SST_IN_KEY_ZDOCID, ZDocIdsVec, ReqProps.GetZDocIdsMapMutable(), zDocId, origZDocId, true);
    }

    void TSaaSRequestRec::DoInsertUrlMasksFromSavedUrl(TStringBuf normUrl, TStringBuf origUrl) {
        GetMasksFromNormalizedDocUrl(MasksBuf, normUrl);
        const size_t oldSz = UrlMasksVec.size();
        for (const auto& fastMask : MasksBuf) {
            fastMask.Render(UrlMaskBuf);
            TStringBuf mask = UrlMaskBuf;
            auto& urlMasksMap = ReqProps.GetUrlMasksMapMutable();
            TStrBufMultiMap::insert_ctx ins;
            TStrBufMultiMap::iterator it = urlMasksMap.find(mask, ins);
            if (it == urlMasksMap.end()) {
                mask = Pool.AppendString(mask);
                UrlMasksVec.emplace_back(mask);
                it = urlMasksMap.insert_direct(std::make_pair(mask, TVector<TStringBuf>()), ins);
            }
            it->second.emplace_back(origUrl);
        }
        if (IsGeneratedKVSubkeys(SST_IN_KEY_URL_MASK)) {
            for (size_t i = oldSz, sz = UrlMasksVec.size(); i < sz; ++i) {
                DoGenerateKVSubkey(SST_IN_KEY_URL_MASK, UrlMasksVec[i]);
            }
        }
    }

    void TSaaSRequestRec::DoInsertUrlNoCgiFromUrl(TStringBuf normUrl, TStringBuf origUrl) {
        UrlNoCgiBuf = GetNoCgiFromNormalizedUrl(normUrl);
        bool inserted = false;
        {
            auto& urlsNoCgiMap = ReqProps.GetUrlsNoCgiMapMutable();
            TStrBufMultiMap::insert_ctx ins;
            TStrBufMultiMap::iterator it = urlsNoCgiMap.find(UrlNoCgiBuf, ins);
            if (it == urlsNoCgiMap.end()) {
                TStringBuf urlNoCgi = UrlNoCgiBuf;
                urlNoCgi = Pool.AppendString(urlNoCgi);
                UrlsNoCgiVec.emplace_back(urlNoCgi);
                it = urlsNoCgiMap.insert_direct(std::make_pair(urlNoCgi, TVector<TStringBuf>()), ins);
                inserted = true;
            }
            it->second.emplace_back(origUrl);
        }

        if (IsGeneratedKVSubkeys(SST_IN_KEY_URL_NO_CGI) && inserted) {
            DoGenerateKVSubkey(SST_IN_KEY_URL_NO_CGI, UrlsNoCgiVec.back());
        }
    }

    void TSaaSRequestRec::DoPrepareStructKeys() {
        if (!PreparedStructKeys) {
            StructKeysVec.clear();
            for (const auto& sk : StructKeys) {
                DoInsertStructKey(sk.first, sk.second);
            }

            for (const auto& sk : StructKeyHierarchy) {
                for (const auto& k : sk.second) {
                    DoInsertStructKey(sk.first, k);
                }
            }

            SortUnique(StructKeysVec);
            PreparedStructKeys = true;
        }
    }

    void TSaaSRequestRec::DoPrepareOwners() {
        if (!PreparedOwners) {
            for (const auto& url : UrlsVec) {
                DoInsertOwner(GetOwnerFromNormalizedDocUrl(url.first), false);
            }
            PreparedOwners = true;
        }
    }

    void TSaaSRequestRec::DoPrepareZDocIds() {
        if (!PreparedZDocIds) {
            for (const auto& url : UrlsVec) {
                DoInsertZDocIdsFromUrl(url.first, url.second);
            }
            PreparedZDocIds = true;
        }
    }

    void TSaaSRequestRec::DoPrepareUrlMasks() {
        if (!PreparedUrlMasks) {
            for (const auto& url : UrlsVec) {
                DoInsertUrlMasksFromSavedUrl(url.first, url.second);
            }
            PreparedUrlMasks = true;
        }
    }

    void TSaaSRequestRec::DoPrepareUrlsNoCgi() {
        if (!PreparedUrlsNoCgi) {
            for (const auto& url : UrlsVec) {
                DoInsertUrlNoCgiFromUrl(url.first, url.second);
            }
            PreparedUrlsNoCgi = true;
        }
    }

    void TSaaSRequestRec::DoPrepareQueryDoppelTokenVec() {
        if (!PreparedQueryDoppelTokenVec) {
            QueryDoppelTokenVec = StringSplitter(QueryDoppel).Split(' ').SkipEmpty().ToList<TStringBuf>();
            PreparedQueryDoppelTokenVec = true;
        }
    }

    void TSaaSRequestRec::DoPrepareQueryDoppelPairVec() {
        if (!PreparedQueryDoppelPairVec) {
            DoPrepareQueryDoppelTokenVec();

            const size_t tokens = QueryDoppelTokenVec.size();
            QueryDoppelPairVec.clear();
            QueryDoppelPairVec.reserve(tokens * tokens + tokens);

            for (auto i = QueryDoppelTokenVec.cbegin(); i != QueryDoppelTokenVec.cend(); ++i) {
                for (auto j = std::next(i); j != QueryDoppelTokenVec.cend(); ++j) {
                    QueryDoppelPairVec.push_back(TString::Join(*i, " ", *j));
                }
                QueryDoppelPairVec.emplace_back(*i);
            }

            PreparedQueryDoppelPairVec = true;
        }
    }

    //////////////
    // TrieSaaS //
    //////////////

    namespace {
        void DoAddRealmKey(NSaasTrie::TRealm& realm, TStringBuf key) {
            TSaaSTrieKey::GenerateRealmKey(*realm.AddKey(), key);
        }
    }

    void TSaaSRequestRec::GenerateTrieKeys(NSaasTrie::TComplexKey& key, const TSaaSKeyType& kt, bool normalSearch, bool urlMaskSearch) {
        Y_ENSURE(KeyTypeIsValid(kt), "invalid key type " << kt);

        if (normalSearch || !urlMaskSearch) {
            TSaaSTrieKey::GenerateMainKey(*key.MutableMainKey(), kt);
        }
        if (urlMaskSearch) {
            key.SetUrlMaskPrefix("." + SubkeyTypeForTrie(SST_IN_KEY_URL_MASK));
            DoPrepareUrlMasks();
        }

        for (auto sst : kt) {
            key.AddKeyRealms(SubkeyTypeForTrie(sst));

            auto& realm = *key.AddAllRealms();
            realm.SetName(SubkeyTypeForTrie(sst));
            switch (sst) {
            default: {
                Y_ENSURE(false, "unsupported subkey type " << (int) sst);
                break;
            }
            case SST_IN_KEY_QUERY_STRONG: {
                DoAddRealmKey(realm, QueryStrong);
                break;
            }
            case SST_IN_KEY_QUERY_DOPPEL: {
                DoAddRealmKey(realm, QueryDoppel);
                break;
            }
            case SST_IN_KEY_QUERY_DOPPEL_TOKEN: {
                DoPrepareQueryDoppelTokenVec();
                for (const auto& token : QueryDoppelTokenVec) {
                    DoAddRealmKey(realm, token);
                }
                break;
            }
            case SST_IN_KEY_QUERY_DOPPEL_PAIR: {
                DoPrepareQueryDoppelPairVec();
                for (const auto& pairStr : QueryDoppelPairVec) {
                    DoAddRealmKey(realm, pairStr);
                }
                break;
            }
            case SST_IN_KEY_USER_ID: {
                DoAddRealmKey(realm, UserId);
                break;
            }
            case SST_IN_KEY_USER_LOGIN_HASH: {
                DoAddRealmKey(realm, UserLoginHash);
                break;
            }
            case SST_IN_KEY_USER_REGION: {
                for (const auto& reg : UserRegionHierarchy) {
                    DoAddRealmKey(realm, reg);
                }
                break;
            }
            case SST_IN_KEY_STRUCT_KEY: {
                DoPrepareStructKeys();
                for (const auto& sk : StructKeysVec) {
                    DoAddRealmKey(realm, sk);
                }
                break;
            }
            case SST_IN_KEY_URL: {
                for (const auto& url : UrlsVec) {
                    DoAddRealmKey(realm, url.first);
                }
                break;
            }
            case SST_IN_KEY_OWNER: {
                DoPrepareOwners();
                for (const auto& owner : OwnersVec) {
                    DoAddRealmKey(realm, owner);
                }
                break;
            }
            case SST_IN_KEY_URL_MASK: {
                DoPrepareUrlMasks();
                for (const auto& urlMask : UrlMasksVec) {
                    DoAddRealmKey(realm, urlMask);
                }
                break;
            }
            case SST_IN_KEY_URL_NO_CGI: {
                DoPrepareUrlsNoCgi();
                for (const auto& urlNoCgi : UrlsNoCgiVec) {
                    DoAddRealmKey(realm, urlNoCgi);
                }
                break;
            }
            case SST_IN_KEY_ZDOCID: {
                DoPrepareZDocIds();
                for (const auto& zDocId : ZDocIdsVec) {
                    DoAddRealmKey(realm, zDocId.first);
                }
                break;
            }
            }
        }
    }

    ////////////
    // KVSaaS //
    ////////////

    void TSaaSRequestRec::GenerateKVSubkeys(ESaaSSubkeyType sst) {
        Y_ENSURE(SubkeyTypeIsValidForKey(sst), "invalid subkey type " << (int)sst);
        if (IsGeneratedKVSubkeys(sst)) {
            return;
        }
        switch (sst) {
        default: {
            Y_ENSURE(false, "unsupported subkey type " << (int) sst);
            break;
        }
        case SST_IN_KEY_QUERY_STRONG: {
            DoGenerateKVSubkey(sst, QueryStrong);
            break;
        }
        case SST_IN_KEY_QUERY_DOPPEL: {
            DoGenerateKVSubkey(sst, QueryDoppel);
            break;
        }
        case SST_IN_KEY_QUERY_DOPPEL_TOKEN: {
            DoPrepareQueryDoppelTokenVec();
            for (const auto& token : QueryDoppelTokenVec) {
                DoGenerateKVSubkey(sst, token);
            }
            break;
        }
        case SST_IN_KEY_QUERY_DOPPEL_PAIR: {
            DoPrepareQueryDoppelPairVec();
            for (const auto& pairStr : QueryDoppelPairVec) {
                DoGenerateKVSubkey(sst, pairStr);
            }
            break;
        }
        case SST_IN_KEY_USER_ID: {
            DoGenerateKVSubkey(sst, UserId);
            break;
        }
        case SST_IN_KEY_USER_LOGIN_HASH: {
            DoGenerateKVSubkey(sst, UserLoginHash);
            break;
        }
        case SST_IN_KEY_USER_REGION: {
            for (const auto& region : UserRegionHierarchy) {
                DoGenerateKVSubkey(sst, region);
            }
            break;
        }
        case SST_IN_KEY_STRUCT_KEY: {
            DoPrepareStructKeys();
            for (const auto& sk : StructKeysVec) {
                DoGenerateKVSubkey(sst, sk);
            }
            break;
        }
        case SST_IN_KEY_URL: {
            for (const auto& url : UrlsVec) {
                DoGenerateKVSubkey(sst, url.first);
            }
            break;
        }
        case SST_IN_KEY_OWNER: {
            DoPrepareOwners();
            for (const auto& owner : OwnersVec) {
                DoGenerateKVSubkey(sst, owner);
            }
            break;
        }
        case SST_IN_KEY_URL_MASK: {
            DoPrepareUrlMasks();
            for (const auto& urlMask : UrlMasksVec) {
                DoGenerateKVSubkey(sst, urlMask);
            }
            break;
        }
        case SST_IN_KEY_URL_NO_CGI: {
            DoPrepareUrlsNoCgi();
            for (const auto& urlNoCgi : UrlsNoCgiVec) {
                DoGenerateKVSubkey(sst, urlNoCgi);
            }
            break;
        }
        case SST_IN_KEY_ZDOCID: {
            DoPrepareZDocIds();
            for (const auto& docId : ZDocIdsVec) {
                DoGenerateKVSubkey(sst, docId.first);
            }
            break;
        }
        }
    }

    class TCounter {
        struct TIndex {
            size_t Idx = 0;
            size_t Size = 0;
            const TVector<TSaaSKVKey>* Values = nullptr;
            ESaaSSubkeyType Type = SST_INVALID;

            TIndex() = default;

            TIndex(ESaaSSubkeyType sst, const TVector<TSaaSKVKey>& values)
                : Size(values.size())
                , Values(&values)
                , Type(sst)
            {}

            TSaaSKVKey GetValue() const {
                return (*Values)[Idx];
            }

            bool Valid() const {
                return Idx < Size;
            }
        };

    public:
        TCounter(const TSaaSKeyType& keyType, const std::array<TVector<TSaaSKVKey>, SST_IN_KEY_COUNT>& values)
        {
            Y_ENSURE(KeyTypeIsValid(keyType), "key type is invalid");
            Indexes.reserve(keyType.size());
            for (auto st : keyType) {
                if (values[st].empty()) {
                    Indexes.clear();
                    break;
                }
                Indexes.emplace_back(TIndex(st, values[st]));
            }
        }

        bool Valid() const {
            return !Indexes.empty() && Indexes.back().Valid();
        }

        void Next() {
            Y_ENSURE(Valid(), "incrementing invalid counter");
            for (auto& idx : Indexes) {
                if (idx.Idx + 1 < idx.Size || &idx == &Indexes.back()) {
                    idx.Idx += 1;
                    break;
                } else {
                    idx.Idx = 0;
                }
            }
        }

        TSaaSKVKey GetValue() const {
            Y_ENSURE(Valid(), "getting invalid value");
            TSaaSKVKey res;
            for (const auto& idx : Indexes) {
                res.Append(idx.GetValue());
            }
            return res;
        }

        size_t Total() const {
            size_t res = Indexes.empty() ? 0 : 1;
            for (const auto& idx : Indexes) {
                res *= idx.Size;
            }
            return res;
        }

    private:
        TVector<TIndex> Indexes;
    };

    void TSaaSRequestRec::GenerateKVKeys(TVector<TSaaSKVKey>& keys, const TSaaSKeyType& kt) {
        Y_ENSURE(KeyTypeIsValid(kt), "invalid key type");

        for (auto st : kt) {
            GenerateKVSubkeys(st);
        }

        TCounter cnt{kt, KVSubkeys};
        keys.reserve(keys.size() + cnt.Total());

        for (; cnt.Valid(); cnt.Next()) {
            keys.emplace_back(cnt.GetValue());
        }
    }

    bool TSaaSRequestRec::IsGeneratedKVSubkeys(ESaaSSubkeyType sst) const {
        return SubkeyTypeIsValidForKey(sst) && !KVSubkeys.at(sst).empty();
    }

    void TSaaSRequestRec::DoClearGeneratedKVSubkeys(ESaaSSubkeyType sst) {
        if (IsGeneratedKVSubkeys(sst)) {
            KVSubkeys[sst].clear();
        }
    }

    void TSaaSRequestRec::DoGenerateKVSubkey(ESaaSSubkeyType sst, TStringBuf key) {
        Y_ENSURE(SubkeyTypeIsValidForKey(sst), "invalid subkey type " << (int)sst);
        if (key) {
            KVSubkeys[sst].emplace_back(TSaaSKVKey::From(sst, key));
        }
    }

    const TStrBufMap& TSaaSReqProps::GetZDocIdsMap() const {
        return ZDocIdsMap;
    }

    const TStrBufMap& TSaaSReqProps::GetUrlsMap() const {
        return UrlsMap;
    }

    const TStrBufMultiMap& TSaaSReqProps::GetUrlsNoCgiMap() const {
        return UrlsNoCgiMap;
    }

    TStrBufMap& TSaaSReqProps::GetZDocIdsMapMutable() {
        return ZDocIdsMap;
    }

    TStrBufMap& TSaaSReqProps::GetUrlsMapMutable() {
        return UrlsMap;
    }

    TStrBufMultiMap& TSaaSReqProps::GetUrlsNoCgiMapMutable() {
        return UrlsNoCgiMap;
    }

    bool TSaaSReqProps::GetSubkeyInferiority(ui32& res, int keyType, const TString& key) const {
        if (const auto* inf = SubkeyInferiority.FindPtr(std::make_pair(keyType, key))) {
            res = *inf;
            return true;
        } else {
            return false;
        }
    }

    bool TSaaSReqProps::GetStructKeyInferiority(ui32& res, const TString& nSpace, const TString& key) const {
        if (const auto* inf = StructKeyInferiority.FindPtr(std::make_pair(nSpace, key))) {
            res = *inf;
            return true;
        } else {
            return false;
        }
    }

    const TSaaSReqProps::TStructKeyInferiority& TSaaSReqProps::GetStructKeyInferiority() const {
        return StructKeyInferiority;
    }

    const TStrBufMultiMap& TSaaSReqProps::GetUrlMasksMap() const {
        return UrlMasksMap;
    };

    TStrBufMultiMap& TSaaSReqProps::GetUrlMasksMapMutable() {
        return UrlMasksMap;
    };

    TSaaSReqProps::TSubkeyInferiority& TSaaSReqProps::GetSubkeyInferiorityMutable() {
        return SubkeyInferiority;
    }

    TSaaSReqProps::TStructKeyInferiority& TSaaSReqProps::GetStructKeyInferiorityMutable() {
        return StructKeyInferiority;
    }

    bool TSaaSReqProps::HasSubkey(int keyType, const TString& key) const {
        return SubkeyInferiority.contains(std::make_pair(keyType, key));
    }

    namespace NProtoHelpers {
        using TProtoMap = NProtoBuf::Map<TString, TString>;
        using TProtoMultiMap = NProtoBuf::Map<TString, NQuerySearch::TSaasReqPropsTransport::TStringArray>;

        TStringBuf AppendString(const TString& s, TMemoryPool& pool) {
            return pool.AppendString(TStringBuf{s});
        }

        void StringMapToProtoMap(const TStrBufMap& strMap, TProtoMap& protoMap) {
            for (const auto& [k, v]: strMap) {
                protoMap[ToString(k)] = ToString(v);
            }
        }
        void ProtoMapToStringMap(const TProtoMap& protoMap, TStrBufMap& strMap, TMemoryPool& pool) {
            for (const auto& [k, v]: protoMap) {
                strMap[AppendString(k, pool)] = AppendString(v, pool);
            }
        }

        void StringMultiMapToProtoMultiMap(const TStrBufMultiMap& strMultiMap, TProtoMultiMap& protoMultiMap) {
            for (const auto& [k, values]: strMultiMap) {
                NQuerySearch::TSaasReqPropsTransport::TStringArray protoValues;
                for (const auto& v: values) {
                    protoValues.AddValue(ToString(v));
                }
                protoMultiMap[ToString(k)] = std::move(protoValues);
            }
        }

        void ProtoMultiMapToStringMultiMap(const TProtoMultiMap& protoMultiMap, TStrBufMultiMap& strMultiMap, TMemoryPool& pool) {
            for (const auto& [k, values]: protoMultiMap) {
                TVector<TStringBuf>& strValues = strMultiMap[AppendString(k, pool)];
                for (size_t i = 0; i < values.ValueSize(); ++i) {
                    const TString& protoValue = values.GetValue(i);
                    TStringBuf v = AppendString(protoValue, pool);
                    strValues.push_back(v);
                }
            }
        }

        template <typename TKey1>
        using TInferiorityMap = THashMap<std::pair<TKey1, TString>, ui32>;

        template <typename TKey1>
        using TProtoInferiorityItem = typename std::conditional<
            std::is_same<TKey1, i32>::value,
            NQuerySearch::TSaasReqPropsTransport::TSubkeyInferiorityValue,
            NQuerySearch::TSaasReqPropsTransport::TStructKeyInferiorityValue
            >::type;


        template <typename TKey1>
        using TProtoInferiorityMap = NProtoBuf::RepeatedPtrField<TProtoInferiorityItem<TKey1>>;

        template <typename TKey1>
        void InferiorityMapToProtoInferiorityMap(const TInferiorityMap<TKey1>& infMap, TProtoInferiorityMap<TKey1>& protoInfMap) {
            for (const auto& [k, v]: infMap) {
                TProtoInferiorityItem<TKey1> item;
                item.SetKey1(k.first);
                item.SetKey2(k.second);
                item.SetValue(v);
                *protoInfMap.Add() = std::move(item);
            }
        }

        template <typename TKey1>
        void ProtoInferiorityMapToInferiorityMap(const TProtoInferiorityMap<TKey1>& protoInfMap, TInferiorityMap<TKey1>& infMap) {
            for (int i = 0; i < protoInfMap.size(); ++i) {
                const auto& item = protoInfMap.Get(i);
                infMap[std::make_pair(item.GetKey1(), item.GetKey2())] = item.GetValue();
            }
        }
    }

    void TSaaSReqProps::Save(IOutputStream* out) const {
        NQuerySearch::TSaasReqPropsTransport transport;

        transport.SetIsRecommendationsRequest(IsRecommendationsRequest);

        NProtoHelpers::StringMapToProtoMap(UrlsMap, *transport.MutableUrlsMap());
        NProtoHelpers::StringMapToProtoMap(ZDocIdsMap, *transport.MutableZDocIdsMap());
        NProtoHelpers::StringMultiMapToProtoMultiMap(UrlsNoCgiMap, *transport.MutableUrlsNoCgiMap());
        NProtoHelpers::StringMultiMapToProtoMultiMap(UrlMasksMap, *transport.MutableUrlMasksMap());
        NProtoHelpers::InferiorityMapToProtoInferiorityMap(SubkeyInferiority, *transport.MutableSubkeyInferiority());
        NProtoHelpers::InferiorityMapToProtoInferiorityMap(StructKeyInferiority, *transport.MutableStructKeyInferiority());

        transport.SerializeToArcadiaStream(out);
    }

    void TSaaSReqProps::Load(IInputStream* in) {
        NQuerySearch::TSaasReqPropsTransport transport;

        transport.ParseFromArcadiaStream(in);

        NProtoHelpers::ProtoMapToStringMap(transport.GetUrlsMap(), UrlsMap, Pool);
        NProtoHelpers::ProtoMapToStringMap(transport.GetZDocIdsMap(), ZDocIdsMap, Pool);
        NProtoHelpers::ProtoMultiMapToStringMultiMap(transport.GetUrlsNoCgiMap(), UrlsNoCgiMap, Pool);
        NProtoHelpers::ProtoMultiMapToStringMultiMap(transport.GetUrlMasksMap(), UrlMasksMap, Pool);
        NProtoHelpers::ProtoInferiorityMapToInferiorityMap(transport.GetSubkeyInferiority(), SubkeyInferiority);
        NProtoHelpers::ProtoInferiorityMapToInferiorityMap(transport.GetStructKeyInferiority(), StructKeyInferiority);
    }
}
