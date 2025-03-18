#pragma once

#include "config_global.h"

#include <antirobot/lib/neh_requester.h>

namespace NAntiRobot {
    template <size_t InstanceIndex>
    class TNehRequesterImpl : public TNehRequester {
    public:
        using TNehRequester::TNehRequester;

        static TNehRequesterImpl& Instance() {
            return *(Singleton<TNehRequesterImpl<InstanceIndex>>(ANTIROBOT_DAEMON_CONFIG.MaxNehQueueSize));
        }
    };

    using TGeneralNehRequester = TNehRequesterImpl<0>;
    using TFuryNehRequester = TNehRequesterImpl<1>;
    using TCaptchaNehRequester = TNehRequesterImpl<2>;
    using TCbbNehRequester = TNehRequesterImpl<3> ;
} // namespace NAntiRobot
