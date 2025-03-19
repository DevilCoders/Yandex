#pragma once

#include "qd_types.h"

#include <util/generic/strbuf.h>

namespace NQueryData {

    struct TSubkeysCounts {
        ui32 Empty = 0;
        ui32 Nonempty = 0;
    };

    TSubkeysCounts GetAllSubkeysCounts(TStringBuf q);

    void SplitKeyIntoSubkeys(TStringBuf q, TStringBufs&);

}
