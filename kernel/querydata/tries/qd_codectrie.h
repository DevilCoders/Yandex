#pragma once

#include "qd_trie.h"

#include <library/cpp/on_disk/codec_trie/codectrie.h>

namespace NQueryData {

    struct TQDCodecTrie : TQDTrie {
        struct TIteratorImpl : TIterator {
            bool Next() override {
                return Iterator.Next();
            }

            bool Current(TString& rawkey, TString& val) const override {
                return Iterator.Current(rawkey, val);
            }

            NCodecTrie::TCodecTrie<TStringBuf>::TConstIterator Iterator;
        };

        void DoInit() override {
            Trie.Init(Blob);
        }

        TString Report() const override {
            return "codectrie: codecs: " + Trie.GetConf().ReportCodecs();
        }

        TAutoPtr<TIterator> Iterator() const override {
            TAutoPtr<TIteratorImpl> it = new TIteratorImpl;
            it->Iterator = Trie.Iterator();
            return it.Release();
        }

        bool Find(TStringBuf rawkey, TValue& v) const override {
            v.Clear();
            return Trie.Get(rawkey, v.Value, v.Buffer1, v.Buffer2);
        }

        bool CanCheckPrefix() const override {
            return Trie.SupportsPrefixLookups();
        }

        bool HasPrefix(TStringBuf prefix) const override {
            return CanCheckPrefix() ? Trie.HasPrefix(prefix) : true;
        }

        NCodecTrie::TCodecTrie<TStringBuf> Trie;
    };

}
