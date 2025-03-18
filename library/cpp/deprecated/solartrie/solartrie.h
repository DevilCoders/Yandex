#pragma once

#include "trie_conf.h"

#include <library/cpp/packers/packers.h>

#include <util/generic/array_ref.h>
#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/memory/blob.h>

namespace NSolarTrie {
    class TSolarTrie {
    public:
        class TImpl;

        class TConstIterator {
            friend class TSolarTrie::TImpl;
            struct TImpl;

            TIntrusivePtr<TImpl> Impl;

        public:
            TConstIterator();
            TConstIterator(const TConstIterator&);
            TConstIterator& operator=(const TConstIterator&);

            ~TConstIterator();

            bool Next();
            bool Current(TBuffer& key, TBuffer& val) const;
            bool HasCurrent() const;
            bool CurrentKey(TBuffer& key) const;
            bool CurrentVal(TBuffer& val) const;
            TStringBuf CurrentKey() const;
            TStringBuf CurrentVal() const;

            void Restart();

            TConstIterator Clone() const;
        };

    public:
        TSolarTrie();
        TSolarTrie(const TSolarTrie&);
        TSolarTrie& operator=(const TSolarTrie&);
        TSolarTrie(TBlob);

        ~TSolarTrie();

        void Init(TBlob);

        bool Get(TStringBuf key, TBuffer& val, TBuffer& auxbuf) const;
        bool Get(TStringBuf key, TBuffer& val) const;

        template <typename TVal>
        bool GetValue(TStringBuf key, TVal& v, TBuffer& valbuff) const {
            return GetValue(key, v, valbuff, NPackers::TPacker<TVal>());
        }

        template <typename TVal>
        bool GetValue(TStringBuf key, TVal& v, TBuffer& valbuff, TBuffer& auxbuf) const {
            return GetValue(key, v, valbuff, auxbuf, NPackers::TPacker<TVal>());
        }

        template <typename TVal, typename TPacker>
        bool GetValue(TStringBuf key, TVal& v, TBuffer& valbuff, const TPacker& p) const {
            if (!Get(key, valbuff))
                return false;
            p.UnpackLeaf(valbuff.Data(), v);
            return true;
        }

        template <typename TVal, typename TPacker>
        bool GetValue(TStringBuf key, TVal& v, TBuffer& valbuff, TBuffer& auxbuf, const TPacker& p) const {
            if (!Get(key, valbuff, auxbuf))
                return false;
            p.UnpackLeaf(valbuff.Data(), v);
            return true;
        }

        TConstIterator Iterator() const;

        size_t Size() const;
        bool Empty() const;

        const TSolarTrieConf& GetConf() const;

        TBlob Data() const;
        // private:
        TIntrusivePtr<TImpl> Impl;
    };

}
