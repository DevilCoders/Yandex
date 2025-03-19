#pragma once

#include <util/generic/vector.h>
#include <util/generic/ptr.h>

#include <util/random/mersenne.h>
#include <util/random/shuffle.h>

template<typename TValueType>
class TShuffleIterator {
private:
    TVector<TValueType*> Values;
    size_t Pointer;

    size_t FoldNumber = 0;
    size_t FoldsCount = 0;

    bool SkipFold = false;
public:
    template <typename TIteratorType>
    TShuffleIterator(TIteratorType begin, TIteratorType end, TMersenne<ui64>& randomGenerator) {
        for (; begin != end; ++begin) {
            Values.push_back(&*begin);
        }
        Shuffle(Values.begin(), Values.end(), randomGenerator);
        Pointer = 0;
    }

    size_t ValuesCount() const {
        return Values.size();
    }

    TShuffleIterator Iterator(const size_t offset) const {
        TShuffleIterator copy(*this);
        copy.Pointer = Min(offset, copy.Values.size());
        return copy;
    }

    TShuffleIterator FoldIterator(const size_t foldNumber, const size_t foldsCount) const {
        TShuffleIterator copy(*this);
        copy.Pointer = copy.FoldNumber = foldNumber;
        copy.FoldsCount = foldsCount;
        copy.SkipFold = false;
        return copy;
    }

    TShuffleIterator SkipIterator(const size_t foldNumber, const size_t foldsCount) const {
        TShuffleIterator copy(*this);
        copy.FoldNumber = foldNumber;
        copy.FoldsCount = foldsCount;
        copy.SkipFold = true;
        copy.Advance();
        return copy;
    }

    TShuffleIterator Begin() const {
        return Iterator(0);
    }

    TShuffleIterator End() const {
        return Iterator(Values.size());
    }

    TValueType& operator*() const {
        return *Values[Pointer];
    }

    TValueType* operator->() const {
        return Values[Pointer];
    }

    bool IsValid() const {
        return Pointer != Values.size();
    }

    TShuffleIterator& operator++() {
        if (IsValid()) {
            ++Pointer;
            Advance();
        }
        return *this;
    }

    TShuffleIterator operator++(int) {
        TShuffleIterator tmp(*this);
        ++tmp;
        return tmp;
    }

    bool operator == (const TShuffleIterator& rhs) const {
        return Pointer == rhs.Pointer;
    }

    bool operator!=(const TShuffleIterator& rhs) const {
        return !operator==(rhs);
    }
private:
    void Advance() {
        if (FoldsCount < 2) {
            return;
        }

        while (IsValid() && (Pointer % FoldsCount == FoldNumber) == SkipFold) {
            ++Pointer;
        }
    }
};

template<typename TValueType>
using TShuffleConstIterator = TShuffleIterator<const TValueType>;
