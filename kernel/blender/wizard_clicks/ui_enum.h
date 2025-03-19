#pragma once

#include <util/generic/serialized_enum.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NWizardsClicks {

    enum EUserInterface {
        UI_DESKTOP  /* "desktop" */,
        UI_PAD  /* "pad" */,
        UI_TOUCH /* "touch" */,
        UI_MOBILE /* "mobile" */,
        UI_MOBILE_APP /* "mobile_app" */,
        UI_ALL /* "all" */,
        UI_UNKNOWN /* "unknown" */
    };

}

