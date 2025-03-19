#pragma once

#include "literal_table.h"

#include <kernel/remorph/core/core.h>

#include <util/charset/wide.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>
#include <utility>

namespace NReMorph {
namespace NPrivate {

class TWcharIterator: public NRemorph::TIteratorBase {
private:
    const wchar16* Symbols;
    size_t Length;

private:
    // Disable default constructor
    TWcharIterator()
        : NRemorph::TIteratorBase()
        , Symbols(nullptr)
        , Length(0)
    {
    }

    TWcharIterator& Proceed() {
        NRemorph::TIteratorBase::Proceed(Symbols ? W16SymbolSize(Symbols + End, Symbols + Length) : 0);
        return *this;
    }

public:
    TWcharIterator(const TWtringBuf& symbols, size_t start = 0)
        : NRemorph::TIteratorBase(start)
        , Symbols(symbols.data())
        , Length(symbols.size())
    {
    }

    inline bool AtBegin() const {
        return 0 == Start && 0 == End;
    }

    inline bool AtEnd() const {
        return End >= Length;
    }

    inline wchar32 GetSymbol() const {
        Y_ASSERT(!AtEnd());
        return ReadSymbol(Symbols + End, Symbols + Length);
    }

    inline bool IsEqual(const TLiteralTable& lt, NRemorph::TLiteral l) {
        Y_ASSERT(!AtEnd());
        return lt.IsEqual(l, GetSymbol());
    }

    inline size_t GetNextCount() const {
        return AtEnd() ? 0 : 1;
    }

    inline TWcharIterator GetNext(size_t n) const {
        Y_UNUSED(n);
        Y_ASSERT(n < GetNextCount());
        return TWcharIterator(*this).Proceed();
    }

    // Returns position range in the original sequence of input symbols
    inline std::pair<size_t, size_t> GetPosRange() const {
        return std::make_pair(Start, End);
    }

    void Swap(TWcharIterator& it) {
        NRemorph::TIteratorBase::Swap(it);
        DoSwap(Symbols, it.Symbols);
        DoSwap(Length, it.Length);
    }
};

} // NPrivate
} // NReMorph
