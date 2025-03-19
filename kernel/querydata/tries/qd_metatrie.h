#pragma once

#include "qd_trie.h"

#include <library/cpp/on_disk/meta_trie/metatrie.h>

namespace NQueryData {

    struct TQDMetatrie : TQDTrie {
        struct TIteratorImpl : TIterator {
            bool Next() override {
                return Iterator.Next();
            }

            bool Current(TString& rawkey, TString& val) const override {
                NMetatrie::TKey k;
                NMetatrie::TVal v;
                bool res = Iterator.Current(k, v);
                rawkey = k.Get();
                val = v.Get();
                return res;
            }

            NMetatrie::TMetatrie::TConstIterator Iterator;
        };

        TQDMetatrie() {}

        void DoInit() override {
            Trie.Init(Blob);
        }

        TString Report() const override {
            return "metatrie: " + Trie.Report();
        }

        TAutoPtr<TIterator> Iterator() const override {
            TAutoPtr<TIteratorImpl> it = new TIteratorImpl;
            it->Iterator = Trie.Iterator();
            return it.Release();
        }

        bool Find(TStringBuf rawkey, TValue& v) const override {
            bool res = Trie.Get(rawkey, v.MVal);
            v.Value = v.MVal.Get();
            return res;
        }

        NMetatrie::TMetatrie Trie;
    };

}
