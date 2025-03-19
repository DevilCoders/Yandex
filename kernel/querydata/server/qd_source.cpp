#include "qd_source.h"

#include <kernel/querydata/client/qd_client_utils.h>
#include <kernel/querydata/request/qd_inferiority.h>

#include <kernel/searchlog/errorlog.h>

#include <util/datetime/base.h>
#include <util/generic/yexception.h>
#include <util/stream/length.h>
#include <util/system/mlock.h>

namespace NQueryData {

    void TSource::LockBlob() {
        SEARCH_INFO << "locking '" << File << "'";
        LockMemory(Blob.AsCharPtr(), Blob.Size());
        MLocked = true;
        SEARCH_INFO << "locked '" << File << "'";
    }

    void TSource::UnlockBlob() {
        if (MLocked) {
            try {
                SEARCH_INFO << "unlocking '" << File << "'";
                UnlockMemory(Blob.AsCharPtr(), Blob.Size());
                MLocked = false;
                SEARCH_INFO << "unlocked '" << File << "'";
            } catch (...) {
                SEARCH_ERROR << CurrentExceptionMessage();
            }
        }
    }

    void TSource::SetUnlockedBlob(TBlob b) {
        UnlockBlob();
        Blob = b;
    }

    void TSource::SetLockedBlob(TBlob b) {
        SetUnlockedBlob(b);
        LockBlob();
    }

    TSource::~TSource() {
        UnlockBlob();
    }

    static ui64 ReadFileDescr(TFileDescription& descr, IInputStream& in) {
        TCountingInput cin(&in);
        TBuffer meta;
        TSerializer<TBuffer>::Load(&cin, meta);
        Y_PROTOBUF_SUPPRESS_NODISCARD descr.ParseFromArray(meta.Data(), meta.Size());
        return cin.Counter();
    }

    static ui64 ReadFileDescr(TFileDescription& descr, const TBlob& data) {
        TMemoryInput min(data.AsCharPtr(), data.Size());
        return ReadFileDescr(descr, min);
    }

    void TSource::Parse(const TBlob& alldata, const TString& file, time_t tstamp) {
        Blob = alldata;
        HeaderLength = ReadFileDescr(Descr, alldata);
        UpdateIndexingTime();
        Y_ENSURE(CountIf(GetAllKeyTypes(Descr), IsPrioritizedNormalization) <= TInferiorityClass::MaxPrioritizedSubkeys32, "too many prioritized keys");
        SetFile(file);
        FileTimestamp = tstamp;
        Trie = SelectTrieImpl(Descr);
        Y_ENSURE(Trie, "failed to read trie '" << File << "'");
    }

    void TSource::InitTrie(ELoadMode lm) {
        Trie->Init(TrieBlob(), lm);
    }

    bool TSource::SupportsFastMMap() const {
        return TrieSupportsFastMMap(Descr);
    }

    ui64 TSource::PredictRAMSize(ELoadMode lm) const {
        return PredictTrieLoadSize(Descr, TrieBlob(), lm);
    }

    NSc::TValue TSource::GetStats(EStatsVerbosity verb) const {
        NSc::TValue stats;

        NSc::TValue& file = stats["file"];
        NSc::TValue& source = stats["source"];
        file["size"] = Size();
        file["timestamp"] = FileTimestamp;
        file["timestamp-hr"] = TimestampToString(FileTimestamp);
        file["load-mode"] = ReportLoadMode();

        source["size"] = ApproxLoadedSize(false);
        source["size-ram"] = ApproxLoadedSize(true);
        source["name"] = Descr.GetSourceName();
        source["timestamp"] = GetTimestampFromVersion(Descr.GetVersion());
        source["timestamp-hr"] = TimestampToString(GetTimestampFromVersion(Descr.GetVersion()));
        source["indexing-timestamp"] = GetTimestampFromVersion(Descr.GetIndexingTimestamp());
        source["indexing-timestamp-hr"] = GetIndexingTimeStr();
        source["shard"] = Sprintf("%u/%u"
                        , Descr.HasShardNumber() ? Descr.GetShardNumber() : 0
                        , Descr.HasShards() ? Descr.GetShards() : 0);

        source["shard-num"] = Descr.GetShardNumber();
        source["shard-total"] = Descr.GetShards();

        if (SV_SOURCES == verb || SV_FACTORS == verb) {
            source["indexer-host"] = Descr.GetIndexerHostName();
            source["command-line"].SetArray();

            for (ui32 i = 0, sz = Descr.IndexerCommandLineOptsSize(); i < sz; ++i) {
                source["command-line"].Push() = Descr.GetIndexerCommandLineOpts(i);
            }

            source["has-json"] = Descr.GetHasJson();
            source["has-keyref"] = Descr.GetHasKeyRef();

            source["norm"] = GetNormalizationNameFromType(Descr.GetKeyType());

            for (ui32 i = 0, sz = Descr.SubkeyTypesSize(); i < sz; ++i) {
                source["auxnorm"][i] = GetNormalizationNameFromType(Descr.GetSubkeyTypes(i));
            }

            source["trie"] = GetTrieVariantNameFromId(Descr.GetTrieVariant());

            if (!!Trie)
                source["compression"] = Trie->Report();

            if (SV_FACTORS == verb) {
                for (size_t i = 0, nf = Descr.FactorsMetaSize(); i < nf; ++i)
                    source["factors"].Add(Descr.GetFactorsMeta(i).GetName());

                for (size_t i = 0, nf = Descr.CommonFactorsSize(); i < nf; ++i)
                    source["common-factors"].Add(Descr.GetCommonFactors(i).GetName());

                if (Descr.HasCommonJson())
                    source["common-json"] = NSc::TValue::FromJson(Descr.GetCommonJson());
            }
        }

        return stats;
    }

    bool TSource::Find(TStringBuf key, TQDTrie::TValue& val, bool exact) const {
        if (!Trie) {
            return false;
        }

        // todo: keyref vs exact
        if (!Descr.GetHasKeyRef() && Descr.GetShards() > 1 && Trie->HasExactKeys()) {
            if (ShardNum(key, Descr.GetShards()) != (ui32)Descr.GetShardNumber()) {
                return false;
            }
        }

        return exact ? Trie->FindExact(key, val) : Trie->Find(key, val);
    }

}
