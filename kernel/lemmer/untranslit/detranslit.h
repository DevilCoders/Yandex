#pragma once

#include <util/generic/fwd.h>

class IDetransliterator {
public:
    virtual ~IDetransliterator() {}
    virtual TUtf16String Convert(const TUtf16String&) const = 0;
};
