#include "qd_saas_yt.h"

#include <kernel/querydata/saas_yt/idl/qd_saas_error_record.pb.h>
#include <kernel/querydata/saas_yt/idl/qd_saas_input_meta.pb.h>
#include <kernel/querydata/saas_yt/idl/qd_saas_input_record.pb.h>
#include <kernel/querydata/saas_yt/idl/qd_saas_snapshot_record.pb.h>

#include <kernel/querydata/common/qd_valid.h>
#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/indexer2/qd_factors_parser.h>
#include <kernel/querydata/saas/qd_saas_kv_key.h>
#include <kernel/querydata/saas/qd_saas_trie_key.h>
#include <kernel/querydata/saas/qd_saas_key_transform.h>
#include <kernel/querydata/server/qd_printer.h>

#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>
#include <library/cpp/json/json_reader.h>

#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/strip.h>

#include <algorithm>

namespace NQueryDataSaaS {

#define QD_SAAS_GENERATE_SUBKEY_GET(type, field) \
    case SST_IN_##type: \
        subkey = in.GetSubkey_##field(); \
        return in.HasSubkey_##field();

#define QD_SAAS_GENERATE_SUBKEY_SET(type, field) \
    case SST_IN_##type: \
        out.SetSubkey_##field(subkey); \
        break;

#define QD_SAAS_COPY_SUBKEY(field) \
    if (entry.HasSubkey_##field()) \
        res.SetSubkey_##field(entry.GetSubkey_##field());

    TQDSaasErrorEntry CreateSaasErrorEntry(const TQDSaaSSnapshotRecord& entry,
        const ui32 httpCode, const TString& errorMessage, ui32 retryNum)
    {

        TQDSaasErrorEntry res;
        QD_SAAS_COPY_SUBKEY(QueryStrong)
        QD_SAAS_COPY_SUBKEY(QueryDoppel)
        QD_SAAS_COPY_SUBKEY(QueryDoppelToken)
        QD_SAAS_COPY_SUBKEY(QueryDoppelPair)
        QD_SAAS_COPY_SUBKEY(Url)
        QD_SAAS_COPY_SUBKEY(Owner)
        QD_SAAS_COPY_SUBKEY(UrlMask)
        QD_SAAS_COPY_SUBKEY(UrlNoCgi)
        QD_SAAS_COPY_SUBKEY(ZDocId)
        QD_SAAS_COPY_SUBKEY(UserLoginHash)
        QD_SAAS_COPY_SUBKEY(UserRegion)
        QD_SAAS_COPY_SUBKEY(UserRegionIpReg)
        QD_SAAS_COPY_SUBKEY(UserIpType)
        QD_SAAS_COPY_SUBKEY(StructKey)
        QD_SAAS_COPY_SUBKEY(SerpTLD)
        QD_SAAS_COPY_SUBKEY(SerpUIL)
        QD_SAAS_COPY_SUBKEY(SerpDevice)

        res.SetSaasErrorCode(httpCode);
        res.SetSaasErrorMessage(errorMessage);
        res.SetRetryCount(retryNum);
        res.SetIsRemoval(false);
        return res;
    }

    namespace {
        template<class TRecord>
        bool DoGetSubkeyFromRecord(TString& subkey, ESaaSSubkeyType st, const TRecord& in) {
            switch (st) {
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_QUERY_STRONG, QueryStrong)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_QUERY_DOPPEL, QueryDoppel)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_QUERY_DOPPEL_TOKEN, QueryDoppelToken)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_QUERY_DOPPEL_PAIR, QueryDoppelPair)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_USER_ID, UserId)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_USER_LOGIN_HASH, UserLoginHash)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_USER_REGION, UserRegion)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_STRUCT_KEY, StructKey)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_URL, Url)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_OWNER, Owner)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_URL_MASK, UrlMask)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_URL_NO_CGI, UrlNoCgi)
            QD_SAAS_GENERATE_SUBKEY_GET(KEY_ZDOCID, ZDocId)
            QD_SAAS_GENERATE_SUBKEY_GET(VALUE_USER_REGION_IPREG, UserRegionIpReg)
            QD_SAAS_GENERATE_SUBKEY_GET(VALUE_USER_IP_TYPE, UserIpType)
            QD_SAAS_GENERATE_SUBKEY_GET(VALUE_SERP_TLD, SerpTLD)
            QD_SAAS_GENERATE_SUBKEY_GET(VALUE_SERP_UIL, SerpUIL)
            QD_SAAS_GENERATE_SUBKEY_GET(VALUE_SERP_DEVICE, SerpDevice)
            default:
                Y_ENSURE(false, (TStringBuilder() << "invalid subkey type " << st));
                return false;
            }
        }

        template<class TRecord>
        void DoSetSubkeyToRecord(TRecord& out, const TString& subkey, ESaaSSubkeyType st) {
            switch (st) {
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_QUERY_STRONG, QueryStrong)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_QUERY_DOPPEL, QueryDoppel)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_QUERY_DOPPEL_TOKEN, QueryDoppelToken)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_QUERY_DOPPEL_PAIR, QueryDoppelPair)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_USER_ID, UserId)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_USER_LOGIN_HASH, UserLoginHash)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_USER_REGION, UserRegion)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_STRUCT_KEY, StructKey)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_URL, Url)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_OWNER, Owner)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_URL_MASK, UrlMask)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_URL_NO_CGI, UrlNoCgi)
            QD_SAAS_GENERATE_SUBKEY_SET(KEY_ZDOCID, ZDocId)
            QD_SAAS_GENERATE_SUBKEY_SET(VALUE_USER_REGION_IPREG, UserRegionIpReg)
            QD_SAAS_GENERATE_SUBKEY_SET(VALUE_USER_IP_TYPE, UserIpType)
            QD_SAAS_GENERATE_SUBKEY_SET(VALUE_SERP_TLD, SerpTLD)
            QD_SAAS_GENERATE_SUBKEY_SET(VALUE_SERP_UIL, SerpUIL)
            QD_SAAS_GENERATE_SUBKEY_SET(VALUE_SERP_DEVICE, SerpDevice)
            default:
                Y_ENSURE(false, (TStringBuilder() << "invalid subkey type " << st));
            }
        }
    }

    bool GetSubkeyFromInputRecord(TString& subkey, ESaaSSubkeyType st, const TQDSaaSInputRecord& in) {
        return DoGetSubkeyFromRecord(subkey, st, in);
    }

    bool GetSubkeyFromSnapshotRecord(TString& subkey, ESaaSSubkeyType st, const TQDSaaSSnapshotRecord& in) {
        return DoGetSubkeyFromRecord(subkey, st, in);
    }

    void SetSubkeyToInputRecord(TQDSaaSInputRecord& out, const TString& subkey, ESaaSSubkeyType st) {
        DoSetSubkeyToRecord(out, subkey, st);
    }

    void SetSubkeyToSnapshotRecord(TQDSaaSSnapshotRecord& out, const TString& subkey, ESaaSSubkeyType st) {
        DoSetSubkeyToRecord(out, subkey, st);
    }

    namespace {
        void DoSetNextSubkey(NQueryData::TSourceFactors& out, const TString& subkey, ESaaSSubkeyType sst) {
            Y_ENSURE(Strip(subkey), "empty subkey of type " << (ESaaSSubkeyType) sst);

            NQueryData::EKeyType kt = GetQueryDataKeyType(sst);
            Y_ENSURE(kt >= 0 && kt < NQueryData::KT_COUNT, (TStringBuilder() << "invalid key type " << (int) kt));

            if (!out.HasSourceKeyType()) {
                out.SetSourceKeyType(kt);
                out.SetSourceKey(subkey);
            } else {
                auto* sk = out.AddSourceSubkeys();
                sk->SetType(kt);
                sk->SetKey(subkey);
            }
        }
    }

    namespace {
        bool DoPrepareInputSubkey(TString& subkey, ESaaSSubkeyType sst) {
            if (!StripInPlace(subkey)) {
                return false;
            }

            if (SST_IN_KEY_URL == sst) {
                subkey = NormalizeDocUrl(subkey);
            } else if (SST_IN_KEY_URL_MASK == sst) {
                auto p = subkey.find(' ');
                if (p != TString::npos) {
                    subkey = subkey.substr(p + 1);
                }
            }

            return true;
        }

        bool DoPrepareSnapshotSubkey(TString& subkey, ESaaSSubkeyType sst, TStringBuf nspace) {
            if (SST_IN_KEY_STRUCT_KEY == sst) {
                subkey = GetStructKeyFromPair(nspace, subkey);
            }

            return true;
        }

        template <typename TSubkeyConsumer>
        TString DoProcessNextSubkey(TQDSaaSSnapshotRecord& out, const TQDSaaSInputRecord& in, ESaaSSubkeyType sst,
            TStringBuf nspace, bool addDebugInfo, TSubkeyConsumer& subkeyConsumer)
        {
            TString subkey;

            if (!GetSubkeyFromInputRecord(subkey, sst, in)) {
                return TString();
            }

            if (!DoPrepareInputSubkey(subkey, sst)) {
                return TString();
            }

            subkeyConsumer.AddSubkey(subkey, sst);

            Y_ENSURE(DoPrepareSnapshotSubkey(subkey, sst, nspace), "invalid subkey '" << subkey << "' for type " << sst);

            if (addDebugInfo) {
                SetSubkeyToSnapshotRecord(out, subkey, sst);
            }

            return subkey;
        }

        template <class TKey, class TSubkeyConsumer>
        std::pair<TString, bool> DoGenerateKey(
            TQDSaaSSnapshotRecord& out, const TQDSaaSInputRecord& in, TStringBuf nspace, bool addDebugInfo,
            TSubkeyConsumer& subkeyConsumer)
        {
            bool hasKey = false;
            TKey key;
            for (auto sst : GetAllTrieSubkeysForSearch()) {
                if (auto subkey = DoProcessNextSubkey(out, in, sst, nspace, addDebugInfo, subkeyConsumer)) {
                    key.Append(sst, subkey);
                    if (SubkeyTypeAllowedInKeyAlone(sst)) {
                        hasKey = true;
                    }
                }
            }
            return std::make_pair(ToString(key), hasKey);
        }

        struct TSubkeyConsumer {
            TSubkeyConsumer(NQueryData::TSourceFactors& sourceFactors, const TVector<ESaaSSubkeyType>* order)
                : SourceFactors(sourceFactors)
                , Order(order)
            {
            }

            void AddSubkey(const TString& subkey, ESaaSSubkeyType sst) {
                if (Order) {
                    Subkeys.emplace_back(sst, subkey);
                } else {
                    DoSetNextSubkey(SourceFactors, subkey, sst);
                }
            }

            void Finalize() {
                if (Order) {
                    std::stable_sort(Subkeys.begin(), Subkeys.end(), [this](const auto& a, const auto& b) {
                        size_t aw = std::distance(Order->begin(), std::find(Order->begin(), Order->end(), a.first));
                        size_t bw = std::distance(Order->begin(), std::find(Order->begin(), Order->end(), b.first));
                        return aw < bw;
                    });
                    for (auto& subkey : Subkeys) {
                        DoSetNextSubkey(SourceFactors, subkey.second, subkey.first);
                    }
                }
            }

        private:
            NQueryData::TSourceFactors& SourceFactors;
            const TVector<ESaaSSubkeyType>* Order = nullptr;
            TVector<std::pair<ESaaSSubkeyType, TString>> Subkeys;
        };
    }

    TSaaSKeyType GetSaaSTrieKeyTypeFromSnapshotRecord(const TQDSaaSSnapshotRecord& in) {
        TString dummy;
        auto& allSubkeyTypes = GetAllTrieSubkeysForSearch();
        TSaaSKeyType keyType;
        keyType.reserve(allSubkeyTypes.size());
        for (auto subkeyType : allSubkeyTypes) {
            if (DoGetSubkeyFromRecord(dummy, subkeyType, in)) {
                keyType.push_back(subkeyType);
            }
        }
        return keyType;
    }

    void FillSnapshotRecordFromInputRecord(TQDSaaSSnapshotRecord& out, const TQDSaaSInputRecord& in, const TQDSaaSInputMeta& meta,
        bool addDebugInfo, EQDSaaSType type)
    {
        TSnapshotRecordFillParams params;
        params.Namespace = meta.GetNamespace();
        params.TimestampMicro = meta.GetTimestamp_Microseconds();
        params.AddDebugInfo = addDebugInfo;
        params.SaasType = type;
        FillSnapshotRecordFromInputRecord(out, in, params);
    }

    void FillSnapshotRecordFromInputRecord(TQDSaaSSnapshotRecord& out, const TQDSaaSInputRecord& in, const TSnapshotRecordFillParams& params)
    {
        ValidateInputMeta(params.Namespace, params.TimestampMicro);
        out.MutableSourceFactors()->SetSourceName(params.Namespace);

        out.SetTimestamp(params.TimestampMicro);
        out.MutableSourceFactors()->SetVersion(params.TimestampMicro);

        TSubkeyConsumer subkeyConsumer(*out.MutableSourceFactors(), params.CustomKeyOrder);
        TString keyStr;
        bool hasKey = false;

        switch (params.SaasType) {
        default:
            Y_ENSURE(false, "invalid type " << (ui32)params.SaasType);
        case EQDSaaSType::KV:
            std::tie(keyStr, hasKey) = DoGenerateKey<TSaaSKVKey>(out, in, params.Namespace, params.AddDebugInfo, subkeyConsumer);
            break;
        case EQDSaaSType::Trie:
            std::tie(keyStr, hasKey) = DoGenerateKey<TSaaSTrieKey>(out, in, params.Namespace, params.AddDebugInfo, subkeyConsumer);
            break;
        }

        Y_ENSURE(hasKey, "Required at least one non-empty column of _IN_KEY_ ones, see ESaaSSubkeyType enum. Namespace " << params.Namespace);
        out.SetSaasKey(keyStr);

        TString prop = TString::Join(QD_SAAS_RECORD_KEY_PREFIX, params.Namespace);
        for (auto sst : GetAllInValueSubkeys()) {
            if (auto subkey = DoProcessNextSubkey(out, in, sst, params.Namespace, params.AddDebugInfo, subkeyConsumer)) {
                prop.append('~').append(subkey);
            }
        }
        out.SetSaasPropName(prop);

        subkeyConsumer.Finalize();

        if (in.HasData_JSON()) {
            NJson::ValidateJsonThrow(in.GetData_JSON());
            out.MutableSourceFactors()->SetJson(in.GetData_JSON());
        } else if (in.HasData_TSKV()) {
            NQueryData::FillSourceFactors(*out.MutableSourceFactors(), in.GetData_TSKV());
        }

        if (params.AddDebugInfo) {
            out.SetNamespace(params.Namespace);
            out.SetSourceFactorsHR(out.GetSourceFactors().ShortUtf8DebugString());
        }

        if (in.GetDelete()) {
            out.SetDelete(true);
        }
    }

    namespace {
        bool DoCopySubkeyFromQDSourceFactors(TQDSaaSInputRecord& out, TString subkey, NQueryData::EKeyType kt) {
            auto sst = GetSaaSKeyType(kt);
            Y_ENSURE(SubkeyTypeIsValid(sst), "unsupported subkey type " << (int) kt);
            Y_ENSURE(DoPrepareInputSubkey(subkey, sst),
                     "invalid subkey of type " << (int) kt << " (" << subkey.Quote() << ")");
            DoSetSubkeyToRecord(out, subkey, sst);
            return SubkeyTypeAllowedInKeyAlone(sst);;
        }
    }

    void FillInputRecordFromQDSourceFactors(TQDSaaSInputRecord& out, const NQueryData::TSourceFactors& in) {
        Y_ENSURE(!in.GetCommon(), "#common records are not supported");
        Y_ENSURE(!in.GetKeyRef(), "#keyref records are not supported");

        if (in.FactorsSize()) {
            TString& tskv = *out.MutableData_TSKV();
            {
                TString buf;
                TStringOutput sout{tskv};
                for (const auto& factor : in.GetFactors()) {
                    if (tskv) {
                        tskv.append('\t');
                    }
                    NQueryData::DumpFactor(sout, buf, factor);
                }
            }
        } else if (auto json = StripString(in.GetJson())) {
            NJson::ValidateJsonThrow(json);
            out.SetData_JSON(json);
        }

        bool hasKey = false;

        if (in.HasSourceKeyType()) {
            hasKey |= DoCopySubkeyFromQDSourceFactors(out, in.GetSourceKey(), in.GetSourceKeyType());
        }

        for (const auto& sk : in.GetSourceSubkeys()) {
            hasKey |= DoCopySubkeyFromQDSourceFactors(out, sk.GetKey(), sk.GetType());
        }

        Y_ENSURE(hasKey, "in-key subkeys required");
    }

    void FillInputMetaFromFileDescription(TQDSaaSInputMeta& meta, const NQueryData::TFileDescription& descr) {
        Y_ENSURE(!descr.GetCommonJson() && !descr.CommonFactorsSize(), "common not allowed");
        meta.Clear();
        meta.SetTimestamp_Microseconds(NQueryData::GetTimestampFromVersion(descr.GetVersion()) * 1'000'000ull);
        meta.SetNamespace(NQueryData::FixSourceNameForSaaS(descr.GetSourceName()));
        ValidateInputMeta(meta);
    }

    void ValidateInputMeta(const TQDSaaSInputMeta& meta) {
        ValidateInputMeta(meta.GetNamespace(), meta.GetTimestamp_Microseconds());
    }

    void ValidateInputMeta(const TString& qdNamespace, ui64 timestampMicro) {
        Y_ENSURE(NQueryData::TimestampMicrosecondsIsValid(timestampMicro), "invalid timestamp " << timestampMicro);
        Y_ENSURE(NQueryData::NamespaceIsValid(qdNamespace), "invalid namespace '" << qdNamespace << "'");
    }
}
