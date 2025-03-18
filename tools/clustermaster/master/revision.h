#pragma once

#include <util/generic/noncopyable.h>
#include <util/system/types.h>

class TRevision: TNonCopyable {
public:
    typedef ui64 TValue;

    TRevision()
        : Revision(Global())
    {
    }

    operator TValue() const {
        return Revision;
    }

    void Up() {
        Revision = ++Global();
    }

private:
    TValue Revision;

    TValue& Global() {
        static TValue Revision = 1;
        return Revision;
    }
};
