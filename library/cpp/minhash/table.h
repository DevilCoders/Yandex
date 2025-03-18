#pragma once

#include "minhash_func.h"

#include <library/cpp/succinct_arrays/eliasfano.h>

#include <util/system/platform.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/bitops.h>
#include <util/ysaveload.h>
#include <library/cpp/streams/factory/factory.h>

namespace NMinHash {
    using namespace NSuccinctArrays;

    template <class T>
    class TColumn {
        typedef TEliasFanoArray<T> TCoder;
        TCoder Coder_;

    public:
        TColumn();
        TColumn(const T& maxOffset, size_t length);
        TColumn(const T* begin, const T* end);
        void Add(const T& value);
        T Get(size_t pos) const;
        T operator[](size_t pos) const;
        void Save(IOutputStream* out) const;
        void Load(IInputStream* inp);
        ui64 Space() const;
    };

    template <class T>
    TColumn<T>::TColumn() {
    }

    template <class T>
    TColumn<T>::TColumn(const T& maxOffset, size_t length)
        : Coder_(maxOffset, length)
    {
    }

    template <class T>
    TColumn<T>::TColumn(const T* begin, const T* end)
        : Coder_(begin, end)
    {
    }

    template <class T>
    void TColumn<T>::Add(const T& value) {
        Coder_.Add(value);
    }

    template <class T>
    T TColumn<T>::Get(size_t pos) const {
        return Coder_.Get(pos);
    }

    template <class T>
    T TColumn<T>::operator[](size_t pos) const {
        return Get(pos);
    }

    template <class T>
    void TColumn<T>::Save(IOutputStream* out) const {
        ::Save(out, Coder_);
    }

    template <class T>
    void TColumn<T>::Load(IInputStream* inp) {
        ::Load(inp, Coder_);
    }

    template <class T>
    ui64 TColumn<T>::Space() const {
        return Coder_.Space();
    }

    template <class T, class H>
    class TTable {
    public:
        typedef H THash;
        typedef T TVal;
        typedef TVector<TAutoPtr<TColumn<TVal>>> TColumns;

    private:
        TColumns Columns_;
        TAutoPtr<H> MinHash_;

    public:
        static const T npos = static_cast<T>(-1);
        TTable();
        TTable(TAutoPtr<H> minHash);
        explicit TTable(IInputStream* inp);
        explicit TTable(const TString& fileName);
        void Add(const T* begin, const T* end);
        template <class I>
        void Add(H* hasher, I begin, I end);
        void Add(TAutoPtr<TColumn<T>> column);
        void AddVal(size_t col, const T& value);
        const TColumn<T>& Get(size_t pos) const;
        T Get(size_t col, size_t row) const;
        T Get(size_t col, const typename H::TKey& key) const;
        T Get(const typename H::TKey& key) const;
        const TColumn<T>& operator[](size_t pos) const;
        const TColumn<T>& operator[](const typename H::TKey& key) const;
        H* Hash() const;
        void SetHash(TAutoPtr<H> hash);
        const TColumns& Columns() const;
        void Save(IOutputStream* out) const;
        void Load(IInputStream* inp);
        ui64 Space() const;
    };

    template <class T, class H>
    TTable<T, H>::TTable()
        : MinHash_(new H())
    {
    }

    template <class T, class H>
    TTable<T, H>::TTable(TAutoPtr<H> minHash)
        : MinHash_(minHash.Release())
    {
    }

    template <class T, class H>
    TTable<T, H>::TTable(IInputStream* inp)
        : MinHash_(new H())
    {
        Load(inp);
    }

    template <class T, class H>
    TTable<T, H>::TTable(const TString& fileName)
        : MinHash_(new H())
    {
        TAutoPtr<IInputStream> inp = OpenInput(fileName);
        Load(inp.Get());
    }

    template <class T, class H>
    void TTable<T, H>::Add(const T* begin, const T* end) {
        TColumn<T>* column = new TColumn<T>(begin, end);
        Columns_.push_back(column);
    }

    template <class T, class H>
    template <class I>
    void TTable<T, H>::Add(H* hasher, I begin, I end) {
        TVector<T> values(hasher->Size());
        for (I it = begin; it != end; ++it) {
            size_t place = hasher->Get(it->first.data(), it->first.size());
            if (Y_UNLIKELY(place >= values.size()))
                ythrow yexception() << "Y_FAIL(" << place << " >= " << values.size() << ")";
            values[place] = it->second;
        }
        TColumn<T>* column = new TColumn<T>(values.begin(), values.end());
        Columns_.push_back(column);
    }

    template <class T, class H>
    void TTable<T, H>::Add(TAutoPtr<TColumn<T>> column) {
        Columns_.push_back(column.Release());
    }

    template <class T, class H>
    void TTable<T, H>::AddVal(size_t col, const T& value) {
        Columns_[col]->Add(value);
    }

    template <class T, class H>
    const TColumn<T>& TTable<T, H>::Get(size_t col) const {
        return *(Columns_[col]);
    }

    template <class T, class H>
    T TTable<T, H>::Get(size_t col, size_t row) const {
        return Get(col)[row];
    }

    template <class T, class H>
    T TTable<T, H>::Get(size_t col, const typename H::TKey& key) const {
        typename H::TDomain row = MinHash_->Get(key);
        if (row == H::npos)
            return npos;
        return Get(col, row);
    }

    template <class T, class H>
    T TTable<T, H>::Get(const typename H::TKey& key) const {
        typename H::TDomain row = MinHash_->Get(~key, +key);
        if (row == H::npos)
            return npos;
        return Get(0, row);
    }

    template <class T, class H>
    const TColumn<T>& TTable<T, H>::operator[](size_t pos) const {
        return Get(pos);
    }

    template <class T, class H>
    const TColumn<T>& TTable<T, H>::operator[](const typename H::TKey& key) const {
        return Get(key);
    }

    template <class T, class H>
    H* TTable<T, H>::Hash() const {
        return MinHash_.Get();
    }

    template <class T, class H>
    void TTable<T, H>::SetHash(TAutoPtr<H> minHash) {
        MinHash_.Reset(minHash.Release());
    }

    template <class T, class H>
    const TVector<TAutoPtr<TColumn<T>>>& TTable<T, H>::Columns() const {
        return Columns_;
    }

    template <class T, class H>
    void TTable<T, H>::Save(IOutputStream* out) const {
        TTableHdr hdr;
        ::Save(out, hdr);
        ::Save(out, *MinHash_);
        ::SaveSize(out, Columns_.size());
        for (size_t i = 0; i < Columns_.size(); ++i)
            ::Save(out, *Columns_[i]);
    }

    template <class T, class H>
    void TTable<T, H>::Load(IInputStream* inp) {
        TTableHdr hdr;
        ::Load(inp, hdr);
        hdr.Verify();
        ::Load(inp, *MinHash_);
        size_t len = ::LoadSize(inp);
        Columns_.resize(len);
        for (size_t i = 0; i < Columns_.size(); ++i) {
            Columns_[i] = new TColumn<T>();
            ::Load(inp, *Columns_[i]);
        }
    }

    template <class T, class H>
    ui64 TTable<T, H>::Space() const {
        ui64 space = 0;
        for (size_t i = 0; i < Columns_.size(); ++i)
            space += Columns_[i]->Space();
        return space;
    }

    using TDefaultTable = TTable<ui64, TChdMinHashFunc>;

}
