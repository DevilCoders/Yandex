#pragma once

#include "intvector.h"

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitops.h>

namespace NSuccinctArrays {
    template <template <typename> class P = std::less>
    struct TPairSecondCmp {
        template <class T1, class T2>
        bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right) {
            return P<T2>()(left.second, right.second);
        }
    };

    template <class TVal = ui64>
    class TFreqValueArray {
        typedef TIntVector TArray;
        THashMap<TVal, size_t> Freq_;
        THolder<TArray> Vals_;
        THolder<TArray> Codes_;

    public:
        TFreqValueArray();
        TFreqValueArray(const TVal* begin, const TVal* end);
        void Learn(const TVal* begin, const TVal* end);
        void Encode(const TVal* begin, const TVal* end);
        void Finish();
        void Add(const TVal& value);
        TVal Get(size_t pos) const;
        TVal operator[](size_t pos) const;
        void Save(IOutputStream* out) const;
        void Load(IInputStream* inp);
        ui64 Space() const;
    };

    template <class TVal>
    TFreqValueArray<TVal>::TFreqValueArray()
        : Vals_(new TArray())
        , Codes_(new TArray())
    {
    }

    template <class TVal>
    TFreqValueArray<TVal>::TFreqValueArray(const TVal* begin, const TVal* end) {
        Learn(begin, end);
        Encode(begin, end);
        Finish();
    }

    template <class TVal>
    void TFreqValueArray<TVal>::Learn(const TVal* begin, const TVal* end) {
        THashMap<TVal, size_t>().swap(Freq_);
        TVal maxVal = 0;
        for (const TVal* it = begin; it != end; ++it) {
            ++Freq_[*it];
            maxVal = Max<TVal>(maxVal, *it);
        }
        TVector<std::pair<TVal, size_t>> codeToValue(Freq_.begin(), Freq_.end());
        Sort(codeToValue.begin(), codeToValue.end(), TPairSecondCmp<std::greater>());
        for (size_t i = 0; i < codeToValue.size(); ++i)
            Freq_[codeToValue[i].first] = i;
        Vals_.Reset(new TArray(0, maxVal ? GetValueBitCount(maxVal) : 0));
        for (size_t i = 0; i < codeToValue.size(); ++i) {
            Vals_->push_back(codeToValue[i].first);
        }
        Codes_.Reset(new TArray(0, codeToValue.size() ? GetValueBitCount(codeToValue.size()) : 0));
    }

    template <class TVal>
    void TFreqValueArray<TVal>::Encode(const TVal* begin, const TVal* end) {
        for (const TVal* it = begin; it != end; ++it)
            Add(*it);
    }

    template <class TVal>
    void TFreqValueArray<TVal>::Finish() {
        THashMap<TVal, size_t>().swap(Freq_);
    }

    template <class TVal>
    void TFreqValueArray<TVal>::Add(const TVal& value) {
        Codes_->push_back(Freq_[value]);
    }

    template <class TVal>
    TVal TFreqValueArray<TVal>::Get(size_t pos) const {
        size_t code = (size_t)(*Codes_)[pos];
        TVal value = (*Vals_)[code];
        return value;
    }

    template <class TVal>
    TVal TFreqValueArray<TVal>::operator[](size_t pos) const {
        return Get(pos);
    }

    template <class TVal>
    void TFreqValueArray<TVal>::Save(IOutputStream* out) const {
        ::Save(out, *Vals_);
        ::Save(out, *Codes_);
    }

    template <class TVal>
    void TFreqValueArray<TVal>::Load(IInputStream* inp) {
        ::Load(inp, *Vals_);
        ::Load(inp, *Codes_);
    }

    template <class TVal>
    ui64 TFreqValueArray<TVal>::Space() const {
        return Vals_->Space() + Codes_->Space();
        //return (Vals_->size() + Codes_->size()) * sizeof(TVal) * CHAR_BIT;
    }

}
