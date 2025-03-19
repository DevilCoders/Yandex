#pragma once

#include "categseries.h"
#include "docsattrs.h"

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>

namespace NGroupingAttrs {

class TDocAttrsSwitcher : TNonCopyable {
public:
    typedef const TCateg* TIterator;

    inline TDocAttrsSwitcher(const TDocsAttrs& attrs, ui32 docid)
        : Attrs_(attrs)
        , DocId_(docid)
    {
    }

    inline void SwitchToAttr(ui32 attrnum) {
        Attrs_.DocCategs(DocId_, attrnum, Categs_);
    }
    inline void SwitchToAttr(const char* attrname) {
        Attrs_.DocCategs(DocId_, attrname, Categs_);
    }

    inline TIterator Begin() const {
        return Categs_.Begin();
    }

    inline TIterator End() const {
        return Categs_.End();
    }

private:
    const TDocsAttrs& Attrs_;
    const ui32 DocId_;
    TCategSeries Categs_;
};

}
