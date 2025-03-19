#pragma once

#include <kernel/search_types/search_types.h>

#include <util/generic/ptr.h>
#include <util/system/defaults.h>

namespace NGroupingAttrs {

class IIterator {
public:
    virtual ~IIterator() {}

    virtual void MoveToAttr(const char* attrname) = 0;
    virtual void MoveToAttr(ui32 attrnum) = 0;
    virtual bool NextValue(TCateg* result) = 0;
};

typedef THolder<IIterator> TIteratorPtr;

}
