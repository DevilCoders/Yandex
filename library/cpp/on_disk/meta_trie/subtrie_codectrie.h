#pragma once

#include "metatrie_impl.h"

#include <library/cpp/on_disk/codec_trie/codectrie.h>
#include <library/cpp/on_disk/codec_trie/triebuilder.h>

namespace NMetatrie {
    using TCodecSubtrie = NCodecTrie::TCodecTrie<TStringBuf>;

    struct TCodecSubtrieIterator : IIterator {
        TCodecSubtrie::TConstIterator Curr;

        TCodecSubtrieIterator(const TCodecSubtrie& trie)
            : Curr(trie.Iterator())
        {
        }

        ESubtrieType GetType() const {
            return ST_CODECTRIE;
        }

        bool Next() override {
            return Curr.Next();
        }

        bool CurrentKey(TKey& k) const override {
            TKeyImpl* key = k.Inner();
            bool res = Curr.CurrentKey(key->Str);
            key->Key = key->Str;
            return res;
        }

        bool CurrentVal(TVal& v) const override {
            TValImpl* val = v.Inner();
            bool res = Curr.CurrentVal(val->Str);
            val->Val = val->Str;
            return res;
        }
    };

    struct TCodecSubtrieWrapper : ITrie {
        TCodecSubtrie Trie;

        TCodecSubtrieWrapper(TBlob b)
            : Trie(b)
        {
        }

        ESubtrieType GetType() const override {
            return ST_CODECTRIE;
        }

        TString Report() const override {
            return Trie.GetConf().ReportCodecs();
        }

        bool Get(TStringBuf key, TVal& val) const override {
            TValImpl* v = val.Inner();
            return Trie.Get(key, v->Val, v->BufAux, v->Buf);
        }

        IIterator* Iterator() const override {
            return new TCodecSubtrieIterator(Trie);
        }
    };

    struct TCodecSubtrieBuilder : TSubtrieBuilderBase {
        typedef NCodecTrie::TCodecTrieBuilder<TStringBuf> TBuilder;
        TBuilder Builder;

        TCodecSubtrieBuilder(NCodecTrie::TCodecTrieConf conf = NCodecTrie::TCodecTrieConf())
            : Builder(conf)
        {
        }

        ESubtrieType GetType() const override {
            return ST_CODECTRIE;
        }

        void DoAdd(TStringBuf key, TStringBuf val) override {
            Builder.Add(key, val);
        }

        TBlob DoCommit() override {
            return Builder.Compact();
        }
    };

}
