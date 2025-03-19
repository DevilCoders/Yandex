#pragma once

#include <util/generic/fwd.h>


namespace NItdItp {

    struct TYabsWidgetLimits {
        static constexpr size_t MaxHosts = 10;

        static constexpr size_t MaxTagsInWhitelistsTotal = 100;
        static constexpr size_t MaxTagsInBlacklistsTotal = 100;
        static constexpr size_t MaxUrlPrefixesInWhitelistsTotal = 100;
        static constexpr size_t MaxUrlPrefixesInBlacklistsTotal = 100;

        static constexpr size_t MaxTagLength = 128;
        static constexpr size_t MaxUrlPrefixLength = 128;
    };

}
