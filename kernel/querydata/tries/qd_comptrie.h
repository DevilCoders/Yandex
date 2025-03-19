#pragma once

#include "qd_trie.h"

#include <library/cpp/containers/comptrie/comptrie_trie.h>

namespace NQueryData {

    // TODO: перенести код реализаций методов в .cpp

    struct TQDCompTrie : TQDTrie {
        struct TIteratorImpl : TIterator {
            bool Next() override {
                if (It.IsEmpty()) {
                    It = Begin;

                    return It != End;
                }

                if (It == End)
                    return false;

                ++It;

                return It != End;
            }

            bool Current(TString& rawkey, TString& val) const override {
                if (It.IsEmpty() || It == End)
                    return false;
                rawkey = It.GetKey();
                val = It.GetValue();
                return true;
            }

            TCompactTrie<char, TStringBuf>::TConstIterator Begin;
            TCompactTrie<char, TStringBuf>::TConstIterator End;
            TCompactTrie<char, TStringBuf>::TConstIterator It;
        };

        void DoInit() override {
            Trie.Init(Blob);
        }

        TString Report() const override {
            return "comptrie: ";
        }

        TAutoPtr<TIterator> Iterator() const override {
            TAutoPtr<TIteratorImpl> it = new TIteratorImpl;
            it->Begin = Trie.Begin();
            it->End = Trie.End();
            return it.Release();
        }

        bool Find(TStringBuf rawkey, TValue& v) const override {
            v.Value.Clear();
            return Trie.Find(rawkey, &v.Value);
        }

        bool CanCheckPrefix() const override { return true; }

        bool HasPrefix(TStringBuf prefix) const override {
            return !Trie.FindTails(prefix).IsEmpty();
        }

        TCompactTrie<char, TStringBuf> Trie;
    };

}
