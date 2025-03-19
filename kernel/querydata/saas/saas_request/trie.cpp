#include "trie.h"

#include <kernel/querydata/cgi/qd_cgi_utils.h>
#include <kernel/querydata/idl/querydata_structs_client.pb.h>
#include <kernel/querydata/saas/qd_saas_request.h>
#include <kernel/saas_trie/idl/saas_trie.pb.h>
#include <kernel/saas_trie/idl/trie_key.h>

#include <cmath>

namespace NQueryDataSaaS {
    namespace {
        ui32 DoCountKeysToSearch(const NSaasTrie::TComplexKey& key) {
            ui32 res = 1;
            for (const auto& realm : key.GetAllRealms()) {
                res *= realm.KeySize();
            }
            return res;
        }

        ui32 DoCountKeysToSend(const NSaasTrie::TComplexKey& key) {
            ui32 res = 0;
            for (const auto& realm : key.GetAllRealms()) {
                res += realm.KeySize();
            }
            return res;
        }

        TSplitDim DoGetSplitDimByMax(const NSaasTrie::TComplexKey& key) {
            TSplitDim res;
            for (ui32 i = 0, sz = key.AllRealmsSize(); i < sz; ++i) {
                const auto& realm = key.GetAllRealms(i);
                if (IsSplittable(NQueryDataSaaS::ParseTrieSubkeyType(realm.GetName())) && realm.KeySize() > res.Weight) {
                    res.Index = i;
                    res.Weight = realm.KeySize();
                }
            }
            return res;
        }
    }

    void GenerateSaasTrieRequests(const TString& client,
                                  const NQueryData::TRequestRec& rec,
                                  const NQuerySearch::TSaaSServiceOpts& opts,
                                  NQuerySearch::TSaaSRequestStats& stats,
                                  IRequestSender& requestSender) {
        TSaaSRequestRec::TRef saasReq{new TSaaSRequestRec};
        saasReq->FillFromQueryData(rec);

        TVector<TSaasKey> reqData;
        const auto& keyTypes = opts.GetKeyTypes();
        reqData.reserve(keyTypes.size());
        const TString additionalCgiParams = saasReq->GenerateCgiFilterParameters();

        ui32 totalWeight = 0;

        for (const auto& keyType : keyTypes) {
            reqData.emplace_back();
            auto& req = reqData.back();
            req.KeyType = keyType;

            auto& keyOpts = opts.GetKeyOpts(keyType);
            saasReq->GenerateTrieKeys(req.Key, ReorderTrieKeyForSearch(keyType), keyOpts.NormalSearch, keyOpts.UrlMaskSearch);
            if (keyOpts.LastRealmUnique) {
                req.Key.SetLastRealmUnique(true);
            }
            if (keyOpts.PrefixSet.to_ullong() != 0) {
                NSaasTrie::WriteRealmPrefixSet(req.Key, keyOpts.PrefixSet);
            }

            req.KeysToSearch = DoCountKeysToSearch(req.Key);
            req.KeysToSend = DoCountKeysToSend(req.Key);
            /* TODO(velavokr): стратегия выбора весов запросов должна быть параметром.
               Стратегия определяет вес типа ключа req.SplitDim.Count.
               Для сервисов, в которых запросы в основном "в молоко" (BANFILTER),
               веса запросов имеет смысл делать пропорциональными числу ключей в наиболее толстом realm.
               Для сервисов, почти каждый запрошенный ключ в которых возвращает результат (QUERYSEARCH-MID-SSD),
               веса запросов имеет смысл делать пропорциональными числу точек в декартовом произведении realm.
             */
            req.SplitDim = DoGetSplitDimByMax(req.Key);

            if (!req.KeysToSearch) {
                continue;
            }

            totalWeight += req.SplitDim.Weight;
        }

        totalWeight = Max(totalWeight, 1u);

        for (auto& req : reqData) {
            auto& keyStats = stats.KeyStats[ToString(req.KeyType)];

            NQueryData::TRequestSplitLimits limits;
            limits.MaxItems = opts.GetMaxKeysInRequest();
            limits.MaxParts = std::ceil(req.SplitDim.Weight * opts.GetMaxRequests() / double(totalWeight));
            limits.MaxParts = Max<ui32>(limits.MaxParts, 1);

            keyStats.KeysToSearchTotal += req.KeysToSearch;
            keyStats.KeysToSendTotal += req.KeysToSend;

            if (auto kps = opts.GetDefaultKps()) {
                req.Key.SetKeyPrefix(kps);
            }

            NQueryData::TRequestSplitMeasure measure;
            measure.SharedSize = 300u;
            measure.SplittableCount = req.SplitDim.Weight;
            measure.SplittableSize = (ui32)req.Key.ByteSize();

            const auto parts = NQueryData::CalculatePartsCount(measure, limits);

            if (parts > 1) {
                NSaasTrie::TRealm realm;

                auto* realmToSplit = req.Key.MutableAllRealms()->Mutable(req.SplitDim.Index);
                realm.SetName(realmToSplit->GetName());
                realm.MutableKey()->Swap(realmToSplit->MutableKey());

                for (ui32 part = 0; part < parts; ++part) {
                    const auto range = NQueryData::GetIndexRange(realm.KeySize(), part, parts);
                    realmToSplit->MutableKey()->Clear();
                    for (ui32 i = range.first; i < range.second; ++i) {
                        realmToSplit->AddKey(realm.GetKey(i));
                    }
                    requestSender.SendRequest(client, req, saasReq, additionalCgiParams);
                    keyStats.ReqsTotal += 1;
                }
            } else {
                requestSender.SendRequest(client, req, saasReq, additionalCgiParams);
                keyStats.ReqsTotal += 1;
            }
        }
    }
}

