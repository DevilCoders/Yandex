#pragma once

#include <kernel/search_types/search_types.h>

#include <util/system/defaults.h>

namespace NSnippets {

struct IGeobase {
    virtual ~IGeobase() {}
    virtual bool Contains(TCateg biggerCateg, TCateg smallerCateg) const = 0;
    virtual TCateg Categ2Parent(TCateg categ) const = 0;
};

}
