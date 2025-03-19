#pragma once

#include "qd_trie.h"

#include <library/cpp/deprecated/solartrie/solartrie.h>

namespace NQueryData {

    struct TQDSolarTrie : TQDTrie {
        struct TIteratorImpl : TIterator {
            bool Next() override {
                return Iterator.Next();
            }

            bool Current(TString& rawkey, TString& val) const override {
                TBuffer k, v;
                bool res = Iterator.Current(k, v);
                rawkey = TStringBuf(k.Data(), k.Size());
                val = TStringBuf(v.Data(), v.Size());
                return res;
            }

            NSolarTrie::TSolarTrie::TConstIterator Iterator;
        };

        TQDSolarTrie() {}

        void DoInit() override {
            Trie.Init(Blob);
        }

        TString Report() const override {
            return "solartrie: codecs: " + Trie.GetConf().ReportCodecs() + "; bucket: " + ToString(Trie.GetConf().BucketMaxSize);
        }

        TAutoPtr<TIterator> Iterator() const override {
            TAutoPtr<TIteratorImpl> it = new TIteratorImpl;
            it->Iterator = Trie.Iterator();
            return it.Release();
        }

        bool Find(TStringBuf rawkey, TValue& v) const override {
            v.Clear();
            bool res = Trie.Get(rawkey, v.Buffer1, v.Buffer2);
            v.Value = TStringBuf(v.Buffer1.Data(), v.Buffer1.Size());
            return res;
        }

        NSolarTrie::TSolarTrie Trie;
    };

}
