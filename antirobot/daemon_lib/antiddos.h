#pragma once

#include "blocker.h"
#include "cbb.h"
#include "request_context.h"
#include "threat_level.h"
#include "user_base.h"

#include <util/generic/ptr.h>

namespace NAntiRobot {
    class TAntiDDoS {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        TAntiDDoS(TBlocker& blocker, ICbbIO* cbbIO);
        ~TAntiDDoS();

        void ProcessRequest(const TRequest& req, TUserBase& userBase, bool isRobot);
        void PrintStats(TStatsWriter& out) const;
    };

    TCbbGroupId BlockCategoryToCbbGroup(EBlockCategory category);
} // namespace NAntiRobot
