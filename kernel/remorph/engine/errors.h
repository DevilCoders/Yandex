#pragma once

#include <kernel/remorph/common/source.h>

#include <util/generic/yexception.h>

namespace NReMorph {

struct TSourceError: public yexception {
    TSourceLocation Location;

    explicit TSourceError(const TSourceLocation& location)
        : Location(location)
    {
        *this << Location << ": ";
    }
};

struct TParsingError: public TSourceError {
    explicit TParsingError(const TSourceLocation& location)
        : TSourceError(location)
    {
    }
};

struct TLexingError: public TSourceError {
    explicit TLexingError(const TSourceLocation& location)
        : TSourceError(location)
    {
    }
};

} // NReMorph
