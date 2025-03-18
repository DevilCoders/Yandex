#pragma once

#include "trie_conf.h"

#include <library/cpp/packers/packers.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/array_ref.h>

#include <util/memory/blob.h>

namespace NSolarTrie {
    // todo: fix interface after akhropov's optimizations
    class TSolarTrieBuilder {
    public:
        explicit TSolarTrieBuilder(TSolarTrieConf conf = TSolarTrieConf());
        ~TSolarTrieBuilder();

        char* Add(TStringBuf key, TStringBuf value);
        char* Add(TStringBuf key, ui32 valuesz);

        // reserve buffer
        char* Add(TStringBuf key, ui32 vSize, TArrayRef<char>& valBuf) {
            char* res = Add(key, vSize);
            valBuf = TArrayRef<char>(res, vSize);
            return res;
        }

        template <typename TVal>
        void AddValue(TStringBuf key, const TVal& v) {
            AddValue(key, v, NPackers::TPacker<TVal>());
        }

        template <typename TVal, typename TPacker>
        void AddValue(TStringBuf key, const TVal& v, const TPacker& p) {
            size_t vSize = p.MeasureLeaf(v);
            char* res = Add(key, vSize);
            p.PackLeaf(res, v, vSize);
        }

        TBlob Compact(); // always destroys data in builder. sorry.

    public:
        class TImpl;

    private:
        TIntrusivePtr<TImpl> Impl;
    };

}
