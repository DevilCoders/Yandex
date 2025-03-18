#pragma once

#include <util/system/defaults.h>

// this file is likely to be move to spec in the future

namespace NHtml {
    enum EAttr {
        HA_any = 0x20,
        HA_class,
        HA_itemscope,
        HA_itemtype
    };

    struct TAttr {
        const char* name;
        const char* LowerName;
        int Id;

        bool operator==(const TAttr& r) const {
            return Id == r.Id;
        }
    };

    const TAttr& FindAttr(const char* name);
    const TAttr& FindAttr(const char* name, size_t len);
    const TAttr* FindAttrPtr(const char* name, size_t len);

}
