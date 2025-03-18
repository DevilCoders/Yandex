#pragma once

#include "mh_heap.h"

#include <util/generic/ptr.h>
#include <util/generic/deque.h>
#include <util/generic/vector.h>

template <typename TIterable>
class TAbstractMerger {
public:
    TAbstractMerger() = default;

    TAbstractMerger(TAbstractMerger&& rhs) = default;
    TAbstractMerger& operator=(TAbstractMerger&& rhs) = default;

    TAbstractMerger(const TAbstractMerger&) = delete;
    void operator=(const TAbstractMerger&) = delete;

    template <typename... Args>
    bool CreateIterable(Args&&... args) {
        auto iterable = MakeHolder<TIterable>(std::forward<Args>(args)...);
        if (!iterable->Valid()) {
            return false;
        }
        Iterables.emplace_back(std::move(iterable));
        return true;
    }

    void Restart() {
        size_t count = Iterables.size();
        HeapIterators.resize(count);
        for (size_t i = 0; i < count; ++i) {
            HeapIterators[i] = Iterables[i].Get();
        }
        Heap.Restart(HeapIterators.data(), count);
    }

    bool Valid() const {
        return HeapIterators.size() > 0 && Heap.Valid();
    }

    TAbstractMerger& operator++() {
        Y_VERIFY(Valid());
        Heap.Next();
        return *this;
    }

    TIterable& Current() const {
        Y_VERIFY(Valid());
        return *Heap.TopIter();
    }

private:
    TDeque<THolder<TIterable>> Iterables;
    TVector<TIterable*> HeapIterators;
    MultHitHeap<TIterable> Heap;
};
