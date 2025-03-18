#pragma once

#include <util/str_stl.h>

#include "base.h"

namespace NHeap {
    template <typename T>
    T& Get(T* val) {
        return *val;
    }

    template <typename T>
    T& Get(T& val) {
        return val;
    }

    template <typename TLess>
    struct TGreater {
        TLess& Less;
        TGreater(TLess& less)
            : Less(less)
        {
        }

        template <typename T>
        bool operator()(const T& a, const T& b) const {
            return Less(Get(b).Current(), Get(a).Current());
        }
    };

    template <typename IterType, typename LessType>
    void Make(IterType begin, IterType end, LessType& less = LessType()) {
        ::MakeHeap(begin, end, TGreater<LessType>(less));
    }

    template <typename IterType, typename LessType>
    void Advance(IterType begin, IterType end, LessType& less = LessType()) {
        IterType child = begin;
        Get(*begin).Advance();
        const bool atEnd = Get(*begin).AtEnd();

        if (
            // Previous heap top is at its end, or
            atEnd ||
            // Top's first child exists and it's greater than top, or
            ((++child) != end && less(Get(*child).Current(), Get(*begin).Current())) ||
            // Top's first child exists, top's second child exists, and it's greater than top
            (child != end && (++child) != end && less(Get(*child).Current(), Get(*begin).Current()))) {
            PopHeap(begin, end, TGreater<LessType>(less));
            if (!atEnd)
                PushHeap(begin, end, TGreater<LessType>(less));
        }
    }
}

template <class T, class L = ::TLess<T>, class TSizer = TSize<T>>
class TExtSort: public TExtAlgorithm<TExtSort<T, L, TSizer>, T, TSizer> {
public:
    typedef L TLess;

private:
    typedef TExtSort<T, TLess, TSizer> TSelf;
    typedef TExtAlgorithm<TSelf, T, TSizer> TBase;

public:
    TExtSort(size_t pagesize = 0, TLess less = TLess(), TSizer sizer = TSizer())
        : TBase(pagesize, sizer)
        , Less(less)
    {
    }

    TExtSort(size_t pagesize, const TString& tmppref, TLess less = TLess(), TSizer sizer = TSizer())
        : TBase(pagesize, tmppref, sizer)
        , Less(less)
    {
    }

    void Swap(TSelf& rhs) {
        TBase::Swap(rhs);
        DoSwap(Less, rhs.Less);
    }

public:
    void PreSave(TVector<T>& buf) {
        Sort(buf.begin(), buf.end(), Less);
    }

    void DoRewind() {
        NHeap::Make(Pages.begin(), Pages.end(), Less);
    }

    void DoAdvance() {
        NHeap::Advance(Pages.begin(), Pages.end(), Less);

        if (Pages.back()->AtEnd()) {
            delete Pages.back();
            Pages.pop_back();
        }
        TBase::Current_ = Pages.begin();
    }

private:
    TLess Less;
    using TBase::Pages;
};
