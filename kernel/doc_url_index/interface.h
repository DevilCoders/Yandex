#pragma once

#include <util/generic/vector.h>
#include <util/generic/strbuf.h>
#include <util/system/types.h>

#include <kernel/indexdoc/omnidoc_fwd.h>

class IDocUrlIndexManager {
public:
    virtual ~IDocUrlIndexManager() {
    }

    virtual TStringBuf Get(const ui32 docId, TOmniUrlAccessor* accessor) const = 0;
    virtual size_t Size() const = 0;
};
