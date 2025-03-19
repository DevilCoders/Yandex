#pragma once

#include "qd_saas_kv_key.h"
#include "qd_saas_trie_key.h"

#include <library/cpp/infected_masks/infected_masks.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <array>
#include <util/generic/deque.h>
#include <util/memory/pool.h>

namespace NQueryData {
    class TQueryData;
    class TInferiorityClass;
    class TRequestRec;
    class TRequestSplitLimits;
}

namespace NSaasTrie {
    class TComplexKey;
}

namespace NQueryDataSaaS {
    using TStrBufSet = THashSet<TStringBuf, THash<TStringBuf>, TEqualTo<TStringBuf>, TPoolAllocator>;
    using TStrBufMap = THashMap<TStringBuf, TStringBuf, THash<TStringBuf>, TEqualTo<TStringBuf>, TPoolAllocator>;
    using TStrBufMultiMap = THashMap<TStringBuf, TVector<TStringBuf>, THash<TStringBuf>, TEqualTo<TStringBuf>, TPoolAllocator>;
    using TStrBufVec = TVector<TStringBuf>;
    using TStrBufPair = std::pair<TStringBuf, TStringBuf>;
    using TStrBufPairVec = TVector<TStrBufPair>;

    class TSaaSReqProps {
    public:
        using TSubkeyInferiority = THashMap<std::pair<int, TString>, ui32>;
        using TStructKeyInferiority = THashMap<std::pair<TString, TString>, ui32>;

        bool GetIsRecommendationsRequest() const {
            return IsRecommendationsRequest;
        }
        void SetIsRecommendationRequest(bool isRecommendationRequest = true) {
            IsRecommendationsRequest = isRecommendationRequest;
        }

        bool GetSubkeyInferiority(ui32&, int qdKeyType, const TString& key) const;
        bool GetStructKeyInferiority(ui32&, const TString& nSpace, const TString& key) const;
        bool HasSubkey(int keyType, const TString& key) const;

        const TStructKeyInferiority& GetStructKeyInferiority() const;
        const TStrBufMap& GetUrlsMap() const;
        const TStrBufMap& GetZDocIdsMap() const;
        const TStrBufMultiMap& GetUrlsNoCgiMap() const;
        const TStrBufMultiMap& GetUrlMasksMap() const;

        TSubkeyInferiority& GetSubkeyInferiorityMutable();
        TStructKeyInferiority& GetStructKeyInferiorityMutable();
        TStrBufMap& GetUrlsMapMutable();
        TStrBufMap& GetZDocIdsMapMutable();
        TStrBufMultiMap& GetUrlsNoCgiMapMutable();
        TStrBufMultiMap& GetUrlMasksMapMutable();

        void Save(IOutputStream* out) const;
        void Load(IInputStream* in);

    private:
        TMemoryPool Pool {1024};

        TStrBufMap UrlsMap {&Pool};
        TStrBufMap ZDocIdsMap {&Pool};
        TStrBufMultiMap UrlsNoCgiMap {&Pool};
        TStrBufMultiMap UrlMasksMap {&Pool};

        TSubkeyInferiority SubkeyInferiority;
        TStructKeyInferiority StructKeyInferiority;

        bool IsRecommendationsRequest = false;
    };

    class TSaaSRequestRec : public TThrRefBase, TNonCopyable {
    public:
        using TRef = TIntrusivePtr<TSaaSRequestRec>;

        TSaaSRequestRec();

        const TString& GetQueryStrong() const; // relev=norm

        const TString& GetQueryDoppel() const; // relev=dnorm

        const TString& GetUserId() const; // rearr=yandexuid || yandexuid

        const TString& GetUserLoginHash() const; // rearr=loginhash || loginhash

        const TVector<TString>& GetUserRegionHierarchy() const; // rearr=all_regions

        const TVector<TString>& GetUserIpRegionHierarchy() const; // rearr=urcntr || rearr=urcntr_debug

        const TVector<TString>& GetUserIpOperatorTypeHierarchy() const; // rearr=gsmop || rearr=gsmop_debug

        const TVector<TString>& GetSerpTopLevelDomainHierarchy() const; // tld

        const TVector<TString>& GetSerpDeviceTypeHierarchy() const; // rearr=device

        const TVector<TString>& GetSerpUILangHierarchy() const; // relev=uil || uil

        const THashSet<std::pair<TString, TString>>& GetStructKeys() const; // rearr=qd_struct_keys

        const THashMap<TString, TVector<TString>>& GetStructKeyHierarchy() const; // rearr=qd_struct_keys

        const TStrBufPairVec& GetUrlsVec() const;

        const TStrBufPairVec& GetZDocIdsVec() const;

        const TStrBufVec& GetOwnersVec() const;

        const TStrBufVec& GetUrlMasksVec() const;

        const TStrBufVec& GetUrlsNoCgiVec() const;

        bool HasSubkey(int qdKeyType, const TString& key) const;

        bool HasStructKey(const TString& nSpace, const TString& key) const;


        const TSaaSReqProps& GetRequestProperties() const {
            return ReqProps;
        }

    public:
        void SetQueryStrong(const TString& queryStrong);

        void SetQueryDoppel(const TString& queryDoppel);

        void SetUserId(const TString& uid);

        void SetUserLoginHash(const TString& loginHash);

        void SetUserRegionHierarchy(const TVector<TString>& smallToBig);

        void SetUserIpRegionHierarchy(const TVector<TString>& smallToBig);

        void SetUserIpOperatorType(const TString& ipType);

        void SetSerpTopLevelDomain(const TString& tld);

        void SetSerpDeviceType(const TString& type);

        void SetSerpUILang(const TString& uil);

        void AddStructKey(const TString& nSpace, const TString& key);

        void AddStructKeyHierarchy(const TString& nSpace, const TVector<TString>& smallToBig);

        void AddUrl(TStringBuf url);

        void AddOwner(TStringBuf owner);

        void AddZDocId(TStringBuf docid);

        void AddFilterNamespace(const TString& filterNamespace);


    public:
        void FillFromQueryData(const NQueryData::TRequestRec&);

    public:
        void GenerateTrieKeys(NSaasTrie::TComplexKey&, const TSaaSKeyType&, bool normalSearch, bool urlMaskSearch);

    public:
        void GenerateKVKeys(TVector<TSaaSKVKey>&, const TSaaSKeyType&);

        void GenerateKVSubkeys(ESaaSSubkeyType);

        bool IsGeneratedKVSubkeys(ESaaSSubkeyType) const;

    public:
        TString GenerateCgiFilterParameters() const;

    private:
        void DoClearGeneratedKVSubkeys(ESaaSSubkeyType);

        void DoGenerateKVSubkey(ESaaSSubkeyType, TStringBuf key);

    private:
        TStrBufPair DoInsertPair(ESaaSSubkeyType, TStrBufPairVec&, TStrBufMap&, TStringBuf item, TStringBuf origItem, bool save);

        void DoInsertOwner(TStringBuf owner, bool save);

        void DoInsertZDocIdsFromUrl(TStringBuf normUrl, TStringBuf origUrl);

        void DoInsertUrlMasksFromSavedUrl(TStringBuf normUrl, TStringBuf origUrl);

        void DoInsertUrlNoCgiFromUrl(TStringBuf normUrl, TStringBuf origUrl);

        void DoInsertStructKey(TStringBuf nSpace, TStringBuf key);

        void DoPrepareStructKeys();

        void DoPrepareOwners();

        void DoPrepareZDocIds();

        void DoPrepareUrlMasks();

        void DoPrepareUrlsNoCgi();

        void DoPrepareQueryDoppelTokenVec();

        void DoPrepareQueryDoppelPairVec();

    private:
        TMemoryPool Pool {1024};

        TString QueryStrong;
        TString QueryDoppel;
        TString UserId;
        TString UserLoginHash;

        TVector<TString> UserRegionHierarchy;
        TVector<TString> UserIpRegionHierarchy;
        TVector<TString> UserIpOperatorTypeHierarchy;
        TVector<TString> SerpTopLevelDomainHierarchy;
        TVector<TString> SerpDeviceTypeHierarchy;
        TVector<TString> SerpUILangHierarchy;

        TStrBufVec StructKeysVec;

        THashSet<std::pair<TString, TString>> StructKeys;
        THashMap<TString, TVector<TString>> StructKeyHierarchy;

        TStrBufPairVec UrlsVec;
        TStrBufMap UrlsMap {&Pool};
        TStrBufPairVec ZDocIdsVec;
        TStrBufMap ZDocIdsMap {&Pool};

        TStrBufVec OwnersVec;
        TStrBufSet OwnersSet {&Pool};

        TStrBufVec UrlMasksVec;
        TStrBufVec UrlsNoCgiVec;

        TSet<TString> FilterNamespacesSet;

        TString UrlBuf;
        TString UrlNoCgiBuf;
        TString ZDocIdBuf0, ZDocIdBuf1;
        TString UrlMaskBuf;
        TString StructKeyBuf;
        TVector<NInfectedMasks::TLiteMask> MasksBuf;

        TStrBufVec QueryDoppelTokenVec;
        TVector<TString> QueryDoppelPairVec;

        bool PreparedStructKeys = false;
        bool PreparedOwners = false;
        bool PreparedZDocIds = false;
        bool PreparedUrlMasks = false;
        bool PreparedUrlsNoCgi = false;
        bool PreparedQueryDoppelTokenVec = false;
        bool PreparedQueryDoppelPairVec = false;

        TSaaSReqProps ReqProps;

    private:
        std::array<TVector<TSaaSKVKey>, SST_IN_KEY_COUNT> KVSubkeys;
    };
}
