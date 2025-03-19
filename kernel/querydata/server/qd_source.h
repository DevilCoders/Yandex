#pragma once

#include "qd_constants.h"

#include <kernel/querydata/common/qi_shardnum.h>
#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/tries/qd_trie.h>
#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/system/fstat.h>
#include <util/folder/path.h>

namespace NQueryData {

    struct TSearchOpts {
        bool SkipNorm = false;
        bool EnableDebugInfo = false;
    };

    class TSource : public TThrRefBase, TNonCopyable {
    public:
        using TPtr = TIntrusivePtr<TSource>;

    private:
        TString File;
        TString FileBaseName;
        time_t FileTimestamp = 0;

        TFileDescription Descr;
        ui64 HeaderLength = 0;
        TString IndexingTimeStr; // for search/web/rearrange/filterbanned/imgban

        TBlob Blob;
        TQDTrie::TPtr Trie;

        bool MLocked = false;

    public:
        ~TSource();

        void LockBlob();

        void UnlockBlob();

        void SetLockedBlob(TBlob b);

        void SetUnlockedBlob(TBlob b);

        const TBlob& GetBlob() const {
            return Blob;
        }

        TString GetFileBaseName() const {
            return FileBaseName;
        }

        const TFileDescription& GetDescr() const {
            return Descr;
        }

        void SetDescr(const TFileDescription& d) {
            Descr.CopyFrom(d);
            UpdateIndexingTime();
        }

        const TString& GetIndexingTimeStr() const {
            return IndexingTimeStr;
        }

        ui64 Size() const {
            return !Trie ? 0 : Blob.Size();
        }


        ui64 ApproxLoadedSize(bool onlyRam) const {
            return !Trie ? 0 : Trie->ApproxLoadedSize(onlyRam);
        }

        void Parse(const TBlob& alldata, const TString& file, time_t tstamp);

        void InitTrie(ELoadMode lm);

        void InitTrie(const TBlob& alldata, const TString& file, time_t tstamp, ELoadMode lm) {
            Parse(alldata, file, tstamp);
            InitTrie(lm);
        }

        void InitFake(const TBlob& alldata, const TString& file, time_t tstamp) {
            InitTrie(alldata, file, tstamp, LM_RAM);
        }

        TBlob TrieBlob() const {
            return Blob.SubBlob(HeaderLength, Blob.Size());
        }

        TQDTrie::TPtr GetTrie() const {
            return Trie;
        }

        void SetTrie(TQDTrie::TPtr t) {
            Trie = t;
        }

        bool SupportsFastMMap() const;

        ui64 PredictRAMSize(ELoadMode lm) const;

        NSc::TValue GetStats(EStatsVerbosity verb) const;

        bool Find(TStringBuf key, TQDTrie::TValue& val, bool exact = false) const;

        bool CanCheckPrefix() const {
            return !Trie ? false : Trie->CanCheckPrefix();
        }

        bool HasPrefix(TStringBuf prefix) const {
            return !Trie ? false : Trie->HasPrefix(prefix);
        }

        ELoadMode GetLoadMode() const {
            return !Trie ? LM_RAM : Trie->GetLoadMode();
        }

        void SetFile(const TString& file, time_t tstamp = 0) {
            File = file;
            FileBaseName = !File ? TString() : TFsPath(File).Basename();
            FileTimestamp = tstamp;
        }

    private:
        TString ReportLoadMode() const {
            switch (!Trie ? LM_NONE : Trie->GetLoadMode()) {
            default:
                return "none";
            case LM_RAM:
                return "ram";
            case LM_FAST_MMAP:
                return "fast-mmap";
            case LM_MMAP:
                return "mmap";
            }
        }

        void UpdateIndexingTime() {
            IndexingTimeStr = TimestampToString(GetTimestampFromVersion(Descr.GetIndexingTimestamp()));
        }
    };

}
