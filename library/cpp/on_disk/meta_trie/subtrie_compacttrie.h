#pragma once

#include "metatrie_impl.h"

#include <library/cpp/containers/comptrie/comptrie.h>

namespace NMetatrie {
    using TCompactSubtrie = TCompactTrie<char, TStringBuf>;

    struct TCompactSubtrieIterator : IIterator {
        const TCompactSubtrie::TConstIterator Begin;
        TCompactSubtrie::TConstIterator Curr;
        const TCompactSubtrie::TConstIterator End;

        TCompactSubtrieIterator(const TCompactSubtrie& trie)
            : Begin(trie.Begin())
            , End(trie.End())
        {
        }

        ESubtrieType GetType() const {
            return ST_COMPTRIE;
        }

        bool Next() override {
            if (End == Curr)
                return false;

            if (Curr.IsEmpty())
                Curr = Begin;
            else
                ++Curr;
            return Curr != End;
        }

        bool CurrentKey(TKey& k) const override {
            if (End == Curr || Curr.IsEmpty())
                return false;

            TKeyImpl* key = k.Inner();
            key->Str = Curr.GetKey();
            key->Key = key->Str;
            return true;
        }

        bool CurrentVal(TVal& v) const override {
            if (End == Curr || Curr.IsEmpty())
                return false;

            TValImpl* val = v.Inner();
            val->Val = Curr.GetValue();
            return true;
        }
    };

    struct TCompactSubtrieWrapper : ITrie {
        TCompactSubtrie Trie;

        TCompactSubtrieWrapper(TBlob b)
            : Trie(b)
        {
        }

        ESubtrieType GetType() const override {
            return ST_COMPTRIE;
        }

        TString Report() const override {
            return TString();
        }

        bool Get(TStringBuf key, TVal& val) const override {
            TValImpl* v = val.Inner();
            return Trie.Find(key.data(), key.size(), &v->Val);
        }

        IIterator* Iterator() const override {
            return new TCompactSubtrieIterator(Trie);
        }
    };

    struct TCompactSubtrieBuilder : TSubtrieBuilderBase {
        typedef TCompactSubtrie::TBuilder TBuilder;
        TBuilder Builder;

        TCompactSubtrieBuilder(ECompactTrieBuilderFlags flags = CTBF_PREFIX_GROUPED)
            : Builder(flags)
        {
        }

        ESubtrieType GetType() const override {
            return ST_COMPTRIE;
        }

        void DoAdd(TStringBuf key, TStringBuf val) override {
            Builder.Add(key.data(), key.size(), val);
        }

        TBlob DoCommit() override {
            TBufferOutput out;
            Builder.Save(out);
            return TBlob::FromBuffer(out.Buffer());
        }
    };

}
