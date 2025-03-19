#pragma once

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>


namespace NGzt {

// From a collection of items selects a sub-set without intersections having
// (1) maximal total size (sum of item sizes)
// (2) minimal number of items if there are several coverages conforming to (1)
//
// TOp should have static methods Size(T), Start(T), Stop(T) returning size_t


template <typename T, typename TOp>
class TCoverage {
public:
    TCoverage()
        : Head(nullptr)
        , Last(nullptr)
        , Size(0)
        , Count(0)
    {
    }

    TCoverage(const TCoverage* head, const T& item)
        : Head(head)
        , Last(&item)
        , Size(TOp::Size(item))
        , Count(1)
    {
        if (head != nullptr) {
            Size += head->TotalSize();
            Count += head->ItemCount();
        }
    }

    size_t ItemCount() const {
        return Count;
    }

    size_t TotalSize() const {
        return Size;
    }

    bool IsBetter(const TCoverage& coverage) const {
        if (TotalSize() != coverage.TotalSize())
            return TotalSize() > coverage.TotalSize();
        return ItemCount() < coverage.ItemCount();
    }

    void ExtractTo(TVector<const T*>& items) const {
        items.resize(Count);
        size_t last = items.size();
        for (const TCoverage* cur = this; cur != nullptr; cur = cur->Head)
            if (cur->Last != nullptr)
                items[--last] = cur->Last;
        Y_ASSERT(last == 0);     // filled all items
    }

    TString DebugString() const {
        TString ret;
        if (Head != NULL)
            ret = Head->DebugString();
        if (Last != NULL) {
            if (!ret.empty())
                ret.append(' ');
            ret += "[" + ::ToString(TOp::Start(*Last)) + ":" + ::ToString(TOp::Stop(*Last)) + "]";
        }
        return ret;
    }

    static inline bool SortPtrPredicate(const T* a, const T* b) {
        // for sorting by right border
        if (TOp::Stop(*a) != TOp::Stop(*b))
            return TOp::Stop(*a) < TOp::Stop(*b);
        return TOp::Start(*a) < TOp::Start(*b);
    }

    static void SelectBestCoverage(TVector<const T*>& items);
    static void SelectBestCoverage(TVector<T>& items);

private:
    const TCoverage* Head;
    const T* Last;
    size_t Size, Count;
};

template <typename T, typename TOp>
void TCoverage<T, TOp>::SelectBestCoverage(TVector<const T*>& items) {
    if (items.empty())
        return;

    // sort segments by right border
    ::StableSort(items.begin(), items.end(), SortPtrPredicate);

    TVector<TCoverage> coverages(items.size());
    coverages[0] = TCoverage(nullptr, *items[0]);
    const TCoverage* bestCoverage = &coverages[0];
    for (size_t i = 1; i < items.size(); ++i) {
        const TCoverage* prevBest = nullptr;    // best coverage, non-overlapping with current @item
        size_t start = TOp::Start(*items[i]);
        for (int j = i - 1; j >= 0; --j)
            if (TOp::Stop(*items[j]) <= start) {
                prevBest = &coverages[j];
                break;
            }

        TCoverage curCoverage(prevBest, *items[i]);
        if (curCoverage.IsBetter(*bestCoverage)) {
            coverages[i] = curCoverage;
            bestCoverage = &coverages[i];
        } else
            coverages[i] = *bestCoverage;
    }

    bestCoverage->ExtractTo(items);
}

template <typename T, typename TOp>
void TCoverage<T, TOp>::SelectBestCoverage(TVector<T>& items) {
    if (items.size() <= 1)  // mega-optimization
        return;

    TVector<const T*> ptrs(items.size());
    for (size_t i = 0; i < items.size(); ++i)
        ptrs[i] = &items[i];

    SelectBestCoverage(ptrs);

    // T should be a light-weight object (easily copyable)
    TVector<T> newItems;
    newItems.reserve(ptrs.size());
    for (size_t i = 0; i < ptrs.size(); ++i)
        newItems.push_back(*ptrs[i]);

    items.swap(newItems);
}

}   // namespace NGzt

