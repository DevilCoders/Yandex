#pragma once

#include <cassert>

#include <algorithm>

#include <util/stream/file.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

#include "iterator.h"

template <typename TIt>
class TMergerIterator: public IIterator<typename TIt::TValueType> {
private:
    typedef typename TIt::TValueType TValueType;
    typedef TVector<TIt*> TIts;
    TIts Its;
    typename TIt::TValueType NextValue;
    bool IsEof;

public:
    template <typename It>
    TMergerIterator(It begin, It end) {
        for (It toIt = begin; toIt != end; ++toIt)
            if (!(*toIt)->Eof())
                Its.push_back(*toIt);

        std::make_heap(Its.begin(), Its.end(), TItCmp<TIt>());
        IsEof = Its.empty();
    }

    inline bool Eof() const override {
        return IsEof;
    }

    const TIt& Top() const {
        assert(!Eof());
        return *(Its.front());
    }

    void Move() override {
        assert(!Eof());
        std::pop_heap(Its.begin(), Its.end(), TItCmp<TIt>());
        Its.back()->Move();
        if (Its.back()->Eof()) {
            Its.pop_back();
        } else {
            std::push_heap(Its.begin(), Its.end(), TItCmp<TIt>());
        }
        IsEof = Its.empty();
    }

    void NowInplace(TValueType& result) const override {
        assert(!Eof());
        Its.front()->NowInplace(result);
    }
};
