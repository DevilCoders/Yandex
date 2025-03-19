#pragma once

#include "qd_trie.h"

#include <library/cpp/on_disk/coded_blob/coded_blob_trie.h>

namespace NQueryData {

    struct TQDCodedBlobTrie : TQDTrie {
        struct TIteratorImpl : TIterator {
            bool Next() override {
                return HasCurrent = Iterator.Next();
            }

            bool Current(TString& rawkey, TString& val) const override {
                if (!HasCurrent)
                    return false;

                rawkey = Iterator.GetCurrentKey();
                val = Iterator.GetCurrentValue();
                return true;
            }

            NCodedBlob::TCodedBlobTrie::TPrimaryKeyIterator Iterator;
            bool HasCurrent = false;
        };

        static NCodedBlob::TCodedBlobTrie::ELoadMode GetTrieLoadMode(ELoadMode lm) {
            switch (lm) {
            default:
                return NCodedBlob::TCodedBlobTrie::LM_RAM;
            case LM_FAST_MMAP:
                return NCodedBlob::TCodedBlobTrie::LM_RAM_MMAP;
            case LM_MMAP:
                return NCodedBlob::TCodedBlobTrie::LM_MMAP;
            }
        }

        void DoInit() override {
            Trie.Init(Blob, GetTrieLoadMode(LoadMode));
        }

        TString Report() const override {
            return Sprintf("codedblobtrie: [keys-sz: %lld, vals-sz: %lld, values %s, mode %s]",
                           (long long)Trie.GetKeysSize(), (long long)Trie.GetDataSize(),
                           Trie.GetCodecName().data(), Trie.ReportLoadMode().data());
        }

        TAutoPtr<TIterator> Iterator() const override {
            TAutoPtr<TIteratorImpl> it = new TIteratorImpl;
            it->Iterator = Trie.PrimaryKeyIterator();
            return it.Release();
        }

        bool Find(TStringBuf rawkey, TValue& v) const override {
            v.Clear();
            return Trie.GetByPrimaryKey(rawkey, v.Value, v.Buffer1);
        }

        ui64 ApproxLoadedSize(bool ramOnly) const override {
            return Trie.ApproxLoadedSize(ramOnly);
        }

        ui64 PredictLoadSize(const TBlob& data, ELoadMode mode) const override {
            return NCodedBlob::TCodedBlobTrie::PredictLoadSize(data, GetTrieLoadMode(mode));
        }

        NCodedBlob::TCodedBlobTrie Trie;
    };

}
