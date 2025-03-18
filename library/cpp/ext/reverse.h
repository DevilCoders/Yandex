#pragma once

#include "base.h"

template <class T, class TSizer = TSize<T>>
class TExtReverse: public TExtAlgorithm<TExtReverse<T, TSizer>, T, TSizer> {
    typedef TExtReverse<T, TSizer> TSelf;
    typedef TExtAlgorithm<TSelf, T, TSizer> TBase;

public:
    TExtReverse(size_t pagesize, TSizer sizer = TSizer())
        : TBase(pagesize, sizer)
    {
    }

    void Swap(TSelf& rhs) {
        TBase::Swap(rhs);
    }

public:
    typename TVector<typename TBase::TPage*>::iterator SelectPage() {
        return TBase::Pages.empty() ? TBase::Pages.end() : TBase::Pages.end() - 1;
    }

    void PreSave(TVector<T>& buf) {
        std::reverse(buf.begin(), buf.end());
    }

    void DoRewind() {
        TBase::Current_ = SelectPage();
    }

    void DoAdvance() {
        (*TBase::Current_)->Advance();
        if ((*TBase::Current_)->AtEnd()) {
            delete *TBase::Current_;
            TBase::Pages.pop_back();
            TBase::Current_ = SelectPage();
        }
    }
};
