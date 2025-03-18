#pragma once

#include <util/system/types.h>

class THttpCookies;

namespace NAntiRobot {
    struct TYxSearchPrefs {
        TYxSearchPrefs()
            : NumDoc(10)
            , Lr(-1)
        {
        }

        void Init(const THttpCookies& cookies);

        ui32 NumDoc;
        // TODO: region by yandex_gid cookie
        i32 Lr;
    };
}
