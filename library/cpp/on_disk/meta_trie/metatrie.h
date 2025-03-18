#pragma once

#include "metatrie_base.h"

namespace NMetatrie {
    class TKey {
    public:
        TStringBuf Get() const;

        TKey();
        TKey(const TKey&);
        TKey& operator=(const TKey&);
        ~TKey();

    protected:
        TIntrusivePtr<TKeyImpl> Impl;

    public:
        TKeyImpl* Inner();
    };

    class TVal {
    public:
        TStringBuf Get() const;

        TVal();
        TVal(const TVal&);
        TVal& operator=(const TVal&);
        ~TVal();

    protected:
        TIntrusivePtr<TValImpl> Impl;

    public:
        TValImpl* Inner();
    };

    class TMetatrie {
    public:
        class TConstIterator {
        public:
            bool Next();
            bool CurrentKey(TKey&) const;
            bool CurrentVal(TVal&) const;

            bool Current(TKey& k, TVal& v) const {
                return CurrentKey(k) && CurrentVal(v);
            }

        public:
            TConstIterator();
            TConstIterator(TIteratorImpl*);
            TConstIterator(const TConstIterator&);
            TConstIterator& operator=(const TConstIterator&);
            ~TConstIterator();

        private:
            TCopyPtr<TIteratorImpl> Impl;
        };

    public:
        void Init(TBlob);
        bool Get(TStringBuf, TVal&) const;
        TConstIterator Iterator() const;

        TString Report() const;

        TBlob Data() const;

    public:
        TMetatrie();
        TMetatrie(TBlob b);
        TMetatrie(const TMetatrie&);
        TMetatrie& operator=(const TMetatrie&);
        ~TMetatrie();

    private:
        TIntrusivePtr<TMetatrieImpl> Impl;
    };

    using TIndexBuilderPtr = TIntrusivePtr<IIndexBuilder>;
    using TSubtrieBuilderPtr = TIntrusivePtr<TSubtrieBuilderBase>;

    class TMetatrieBuilder : TNonCopyable {
    public:
        TMetatrieBuilder();
        TMetatrieBuilder(IOutputStream& out, TIndexBuilderPtr b);
        ~TMetatrieBuilder();

        void Init(IOutputStream& out, TIndexBuilderPtr b);

        void AddSubtrie(TStringBuf firstkey, TStringBuf lastkey, ESubtrieType, TBlob blob);
        void AddSubtrie(TStringBuf firstkey, TStringBuf lastkey, ESubtrieType, TStringBuf blob);
        void AddSubtrie(TSubtrieBuilderBase&);
        void CommitAll();

    private:
        struct TImpl;
        THolder<TImpl> Impl;
    };

    TIndexBuilderPtr CreateIndexBuilder(const TMetatrieConf& conf, ui64 nportions, ui64 hashedPrefixLength = TStringBuf::npos);
    TSubtrieBuilderPtr CreateSubtrieBuilder(const TMetatrieConf& conf);

}
