#include "qd_saas_key_transform.h"
#include "qd_saas_kv_key.h"
#include "qd_saas_response.h"
#include "qd_saas_request.h"
#include "qd_saas_trie_key.h"

#include <kernel/querydata/idl/querydata_common.pb.h>
#include <kernel/querydata/idl/querydata_structs_client.pb.h>
#include <kernel/querydata/request/qd_inferiority.h>

#include <kernel/urlid/url2docid.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/deque.h>
#include <util/string/builder.h>
#include <util/system/src_location.h>
#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/client/qd_merge.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NQueryDataSaaS {
    namespace {
        bool DoCheckRequestAcceptsSubkey(NQueryData::EKeyType subkeyType, const TString& subkey, const TSaaSReqProps& props) {
            if (SubkeyTypeIsValidForKey(GetSaaSKeyType(subkeyType)) || props.GetIsRecommendationsRequest()) {
                return true;
            }
            switch (subkeyType) {
            default:
                return false;
            case NQueryData::KT_USER_REGION_IPREG:
            case NQueryData::KT_USER_IP_TYPE:
            case NQueryData::KT_SERP_TLD:
            case NQueryData::KT_SERP_UIL:
            case NQueryData::KT_SERP_DEVICE:
                return props.HasSubkey(subkeyType, subkey);
            }
        }

        bool DoCheckRequestAcceptsRecord(const NQueryData::TSourceFactors& sf, const TSaaSReqProps& props) {
            if (sf.HasSourceKeyType() && !DoCheckRequestAcceptsSubkey(sf.GetSourceKeyType(), sf.GetSourceKey(), props)) {
                return false;
            }
            for (const auto& sk : sf.GetSourceSubkeys()) {
                if (!DoCheckRequestAcceptsSubkey(sk.GetType(), sk.GetKey(), props)) {
                    return false;
                }
            }
            return true;
        }

        const TStrBufMap* DoGetUrlRestoreMap(NQueryData::EKeyType kt, const TSaaSReqProps& props) {
            auto sst = GetSaaSKeyType(kt);
            if (SST_IN_KEY_URL == sst) {
                return &props.GetUrlsMap();
            } else if (SST_IN_KEY_ZDOCID == sst) {
                return &props.GetZDocIdsMap();
            } else {
                return nullptr;
            }
        }

        void DoRestoreOrigSubkey(TString& subkey, const TStrBufMap& map) {
            if (const auto* val = map.FindPtr(subkey)) {
                subkey.assign(*val);
            }
        }

        void DoRestoreOrigUrls(NQueryData::TSourceFactors& sf, const TSaaSReqProps& props) {
            if (const auto* map = DoGetUrlRestoreMap(sf.GetSourceKeyType(), props)) {
                DoRestoreOrigSubkey(*sf.MutableSourceKey(), *map);
            }
            for (auto& sk : *sf.MutableSourceSubkeys()) {
                if (const auto* map = DoGetUrlRestoreMap(sk.GetType(), props)) {
                    DoRestoreOrigSubkey(*sk.MutableKey(), *map);
                }
            }
        }

        bool DoCalculateSubkeyPriority(
            NQueryData::TInferiorityClass& inf, const TString& nSpace,
            NQueryData::EKeyType subkeyType, const TString& subkey, const TSaaSReqProps& props)
        {
            ui32 cnt = 0;
            if (NQueryData::IsPrioritizedNormalization(subkeyType) && props.GetSubkeyInferiority(cnt, subkeyType, subkey)
                || (NQueryData::KT_STRUCTKEY == subkeyType && props.GetStructKeyInferiority(cnt, nSpace, subkey)))
            {
                inf.AddSubinferiority((ui8)cnt);
                return true;
            } else {
                return false;
            }
        }

        void DoCalculateRecordPriority(NQueryData::TSourceFactors& sf, const TSaaSReqProps& props) {
            NQueryData::TInferiorityClass inf;

            if (sf.HasSourceKeyType()) {
                if (DoCalculateSubkeyPriority(inf, sf.GetSourceName(), sf.GetSourceKeyType(), sf.GetSourceKey(), props)) {
                    sf.MutableSourceKeyTraits()->SetIsPrioritized(true);
                }
            }
            for (auto& sk : *sf.MutableSourceSubkeys()) {
                if (DoCalculateSubkeyPriority(inf, sf.GetSourceName(), sk.GetType(), sk.GetKey(), props)) {
                    sk.MutableTraits()->SetIsPrioritized(true);
                }
            }
            sf.MutableMergeTraits()->SetPriority(inf.GetPriority64());
        }

        void DoMergeRecords(TDeque<NQueryData::TSourceFactors>& merged, TDeque<NQueryData::TSourceFactors>& records) {
            NQueryData::MergeQueryDataSimple(merged, records);
        }

        bool SubkeyIsUrlMask(NQueryData::EKeyType kt) {
            return IsIn({NQueryData::KT_CATEG_URL, NQueryData::KT_SNIPCATEG_URL}, kt);
        }

        bool RecordHasUrlMask(const NQueryData::TSourceFactors& sf) {
            if (SubkeyIsUrlMask(sf.GetSourceKeyType())) {
                return true;
            }
            for (const auto& sk : sf.GetSourceSubkeys()) {
                if (SubkeyIsUrlMask(sk.GetType())) {
                    return true;
                }
            }
            return false;
        }

        bool RecordHasUrlNoCgi(const NQueryData::TSourceFactors& sf) {
            if (NQueryData::KT_URL_NO_CGI == sf.GetSourceKeyType()) {
                return true;
            }
            for (const auto& sk : sf.GetSourceSubkeys()) {
                if (NQueryData::KT_URL_NO_CGI == sk.GetType()) {
                    return true;
                }
            }
            return false;
        }

        bool DoExpandUrlMask(
            NQueryData::TQueryData& res, const NQueryData::TSourceFactors& sf, const TSaaSReqProps& props)
        {
            // Маска в ключе может быть только одна
            if (SubkeyIsUrlMask(sf.GetSourceKeyType())) {
                if (const auto* urls = props.GetUrlMasksMap().FindPtr(sf.GetSourceKey())) {
                    for (size_t i = 0, sz = urls->size(); i < sz; ++i) {
                        auto sfOut = res.AddSourceFactors();
                        sfOut->CopyFrom(sf);
                        sfOut->SetSourceKey(TString{(*urls)[i]});
                        sfOut->SetSourceKeyType(NQueryData::KT_URL);
                    }
                    return true;
                } else {
                    return false;
                }
            } else {
                for (size_t k = 0, ksz = sf.SourceSubkeysSize(); k < ksz; ++k) {
                    if (SubkeyIsUrlMask(sf.GetSourceSubkeys(k).GetType())) {
                        if (const auto* urls = props.GetUrlMasksMap().FindPtr(sf.GetSourceSubkeys(k).GetKey())) {
                            for (size_t i = 0, sz = urls->size(); i < sz; ++i) {
                                auto sfOut = res.AddSourceFactors();
                                sfOut->CopyFrom(sf);
                                auto& sk = *sfOut->MutableSourceSubkeys(k);
                                sk.SetKey(TString{(*urls)[i]});
                                sk.SetType(NQueryData::KT_URL);
                            }
                            return true;
                        } else {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        bool DoExpandUrlNoCgi(
            NQueryData::TQueryData& res, const NQueryData::TSourceFactors& sf, const TSaaSReqProps& props)
        {
            //TODO: less copypasta with DoExpandUrlMask
            if (NQueryData::KT_URL_NO_CGI == sf.GetSourceKeyType()) {
                if (const auto* urls = props.GetUrlsNoCgiMap().FindPtr(sf.GetSourceKey())) {
                    for (size_t i = 0, sz = urls->size(); i < sz; ++i) {
                        auto sfOut = res.AddSourceFactors();
                        sfOut->CopyFrom(sf);
                        sfOut->SetSourceKey(TString{(*urls)[i]});
                        sfOut->SetSourceKeyType(NQueryData::KT_URL);
                    }
                    return true;
                } else {
                    return false;
                }
            } else {
                for (size_t k = 0, ksz = sf.SourceSubkeysSize(); k < ksz; ++k) {
                    if (NQueryData::KT_URL_NO_CGI == sf.GetSourceSubkeys(k).GetType()) {
                        if (const auto* urls = props.GetUrlsNoCgiMap().FindPtr(sf.GetSourceSubkeys(k).GetKey())) {
                            for (size_t i = 0, sz = urls->size(); i < sz; ++i) {
                                auto sfOut = res.AddSourceFactors();
                                sfOut->CopyFrom(sf);
                                auto& sk = *sfOut->MutableSourceSubkeys(k);
                                sk.SetKey(TString{(*urls)[i]});
                                sk.SetType(NQueryData::KT_URL);
                            }
                            return true;
                        } else {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

#define QD_SAAS_LOG_RESPONSE_ERROR_LOC(error, loc) do { \
        TStringBuilder err; \
        err << error << " (" << loc << ")"; \
        stats.Errors.emplace_back(err); \
        stats.RecordsRejectedAsMalformed += 1; \
    } while (false)

#define QD_SAAS_LOG_RESPONSE_ERROR(error) QD_SAAS_LOG_RESPONSE_ERROR_LOC(error, __LOCATION__)

        void DoFinalizeRecords(NQueryData::TQueryData& res, TDeque<NQueryData::TSourceFactors>& merged, const TSaaSReqProps& props, TProcessingStats& stats) {
            for (auto& sf : merged) {
                if (RecordHasUrlMask(sf)) {
                    if (!DoExpandUrlMask(res, sf, props)) {
                        QD_SAAS_LOG_RESPONSE_ERROR("could not expand mask");
                        continue;
                    }
                } else if (RecordHasUrlNoCgi(sf)) {
                    if (!DoExpandUrlNoCgi(res, sf, props)) {
                        QD_SAAS_LOG_RESPONSE_ERROR("could not process url-no-cgi");
                        continue;
                    }
                } else {
                    res.AddSourceFactors()->Swap(&sf);
                }
            }
        }

    } //namespace

    TProcessingStats& TProcessingStats::operator += (const TProcessingStats& other) {
        RecordsRejectedByRequest += other.RecordsRejectedByRequest;
        RecordsRejectedByMerge += other.RecordsRejectedByMerge;
        RecordsRejectedAsMalformed += other.RecordsRejectedAsMalformed;
        RecordsAddedByFinalization += other.RecordsAddedByFinalization;
        Errors.insert(Errors.end(), other.Errors.begin(), other.Errors.end());
        return *this;
    }

    TProcessingStats ProcessSaaSResponse(NQueryData::TQueryData& res, const TSaaSDocument& doc, const TSaaSReqProps& props) {
        TProcessingStats stats;

        TDeque<NQueryData::TSourceFactors> records;
        NQueryData::TSourceFactors sf;

        TString base64Buf;
        for (size_t recIdx = 0, recCnt = doc.Records.size(); recIdx < recCnt; ++recIdx) {
            const auto& rawRec = doc.Records[recIdx];

            try {
                Base64Decode(rawRec, base64Buf);
            } catch (const yexception& e) {
                QD_SAAS_LOG_RESPONSE_ERROR("could not decode base64 at idx=" << recIdx << " key=" << doc.Key);
                continue;
            }

            if (!sf.ParseFromString(base64Buf)) {
                QD_SAAS_LOG_RESPONSE_ERROR("could not parse record at idx=" << recIdx << " key=" << doc.Key);
                continue;
            }

            if (!sf.GetSourceName()) {
                QD_SAAS_LOG_RESPONSE_ERROR("empty source name at idx=" << recIdx << " key=" << doc.Key);
                continue;
            }

            if (!DoCheckRequestAcceptsRecord(sf, props)) {
                stats.RecordsRejectedByRequest += 1;
                continue;
            }

            DoRestoreOrigUrls(sf, props);
            DoCalculateRecordPriority(sf, props);
            records.emplace_back().CopyFrom(sf);
        }

        TDeque<NQueryData::TSourceFactors> mergedRecords;
        DoMergeRecords(mergedRecords, records);
        stats.RecordsRejectedByMerge = (ui32)(records.size() - mergedRecords.size());

        const size_t oldSize = res.SourceFactorsSize();
        DoFinalizeRecords(res, mergedRecords, props, stats);
        stats.RecordsAddedByFinalization = ui32((res.SourceFactorsSize() - oldSize) - mergedRecords.size());

        return stats;
    }

}
