#pragma once

#include "metatrie_impl.h"

#include <library/cpp/deprecated/solartrie/solartrie.h>
#include <library/cpp/deprecated/solartrie/triebuilder.h>

namespace NMetatrie {
    struct TSolarSubtrieIterator : IIterator {
        NSolarTrie::TSolarTrie::TConstIterator Curr;

        TSolarSubtrieIterator(const NSolarTrie::TSolarTrie& trie)
            : Curr(trie.Iterator())
        {
        }

        ESubtrieType GetType() const {
            return ST_SOLARTRIE;
        }

        bool Next() override {
            return Curr.Next();
        }

        bool CurrentKey(TKey& k) const override {
            TKeyImpl* key = k.Inner();
            bool res = Curr.CurrentKey(key->Buf);
            key->Key = TStringBuf(key->Buf.data(), key->Buf.size());
            return res;
        }

        bool CurrentVal(TVal& v) const override {
            TValImpl* val = v.Inner();
            bool res = Curr.CurrentVal(val->Buf);
            val->Val = TStringBuf(val->Buf.data(), val->Buf.size());
            return res;
        }
    };

    struct TSolarSubtrieWrapper : ITrie {
        NSolarTrie::TSolarTrie Trie;

        TSolarSubtrieWrapper(TBlob b)
            : Trie(b)
        {
        }

        ESubtrieType GetType() const override {
            return ST_SOLARTRIE;
        }

        TString Report() const override {
            return Trie.GetConf().ReportCodecs();
        }

        bool Get(TStringBuf key, TVal& val) const override {
            TValImpl* v = val.Inner();
            bool res = Trie.Get(key, v->Buf, v->BufAux);
            v->Val = TStringBuf(v->Buf.data(), v->Buf.size());
            return res;
        }

        IIterator* Iterator() const override {
            return new TSolarSubtrieIterator(Trie);
        }
    };

    struct TSolarSubtrieBuilder : TSubtrieBuilderBase {
        NSolarTrie::TSolarTrieBuilder Builder;

        TSolarSubtrieBuilder(NSolarTrie::TSolarTrieConf conf = NSolarTrie::TSolarTrieConf())
            : Builder(conf)
        {
        }

        ESubtrieType GetType() const override {
            return ST_SOLARTRIE;
        }

        void DoAdd(TStringBuf key, TStringBuf val) override {
            Builder.Add(key, val);
        }

        TBlob DoCommit() override {
            return Builder.Compact();
        }
    };

}
