#pragma once

#include <kernel/querydata/common/qd_util.h>

#include <library/cpp/on_disk/meta_trie/metatrie.h>

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/memory/blob.h>
#include <util/system/align.h>
#include <util/system/madvise.h>
#include <util/system/info.h>

namespace NQueryData {

    enum ELoadMode {
        LM_NONE = 0,
        LM_RAM, // all in ram
        LM_FAST_MMAP, // hot parts in ram, cold are mmapped
        LM_MMAP, // all mmapped
    };

    class TQDTrie : public TThrRefBase {
    public:
        using TPtr = TIntrusivePtr<TQDTrie>;

        struct TValue {
            TStringBuf Get() const {
                return Value;
            }

            void Clear();

            // ugly but simple and allows reuse
            TStringBuf Value;
            TBuffer Buffer1;
            TBuffer Buffer2;
            TString StrVal;
            NMetatrie::TVal MVal;
        };

        struct TIterator : TNonCopyable {
            virtual bool Next() = 0;
            virtual bool Current(TString& rawkey, TString& val) const = 0;
            virtual ~TIterator() {}
        };

        void Init(const TBlob& trie, ELoadMode lm) {
            Blob = trie;
            LoadMode = lm;
            DoInit();
        }

        virtual void DoInit() = 0;
        virtual TString Report() const = 0;

        virtual TAutoPtr<TIterator> Iterator() const = 0;
        virtual bool Find(TStringBuf rawkey, TValue&) const = 0;

        virtual bool FindExact(TStringBuf rawkey, TValue& val) const {
            return Find(rawkey, val);
        }

        virtual bool CanCheckPrefix() const { return false; }
        virtual bool HasExactKeys() const { return true; }

        virtual bool HasPrefix(TStringBuf) const { return false; }

        virtual ui64 ApproxLoadedSize(bool ramOnly) const {
            return ((ramOnly && LM_RAM == LoadMode) || !ramOnly) ? Size() : 0;
        }

        virtual ui64 Size() const {
            return Blob.Size();
        }

        virtual ui64 PredictLoadSize(const TBlob& data, ELoadMode lm) const {
            return LM_RAM == lm ? data.Size() : 0;
        }

        ELoadMode GetLoadMode() const {
            return LoadMode;
        }

        ~TQDTrie() override {
            if (Blob.Size() > 10 * NSystemInfo::GetPageSize() && (LM_MMAP == LoadMode || LM_FAST_MMAP == LoadMode)) {
                MadviseEvict(AlignUp(Blob.AsCharPtr(), NSystemInfo::GetPageSize()), AlignDown(Blob.Size(), NSystemInfo::GetPageSize()));
            }
        }

    protected:
        TBlob Blob;
        ELoadMode LoadMode = LM_RAM;
    };

    class TFileDescription;

    TQDTrie::TPtr SelectTrieImpl(const TFileDescription& d);

    bool TrieSupportsFastMMap(const TFileDescription& d);
    ui64 PredictTrieLoadSize(const TFileDescription& d, const TBlob& trie, ELoadMode lm);

}
