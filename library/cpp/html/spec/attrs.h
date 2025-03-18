#pragma once

#include <util/system/defaults.h>

enum class EAttrNS : i8 {
    NONE,
    XLINK,
    XML,
    XMLNS,
};

namespace NHtml {
    struct TExtent {
        ui32 Start;
        ui32 Leng;

        TExtent()
            : Start(0)
            , Leng(0)
        {
        }
    };

    struct TAttribute {
        TExtent Name;
        TExtent Value;
        char Quot;
        EAttrNS Namespace;

        TAttribute()
            : Name()
            , Value()
            , Quot(0)
            , Namespace(EAttrNS::NONE)
        {
        }

        bool IsBoolean() const {
            return Name.Start == Value.Start;
        }

        bool IsQuoted() const {
            return Quot != 0;
        }
    };

}
