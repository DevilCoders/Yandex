#pragma once

#include "antirobot_experiment.h"
#include "amnesty_flags.h"
#include "disabling_flags.h"
#include "disabling_stat.h"
#include "its_file_handler.h"
#include "many_requests.h"
#include "panic_flags.h"
#include "server_error_stats.h"

#include <antirobot/daemon_lib/config_global.h>
#include <antirobot/lib/alarmer.h>

#include <atomic>

namespace NAntiRobot {
    class TItsFilesWatcher {
    public:
        using TServiceHandleFileHandler = TItsFileHandler<TServiceParamHolder<TAtomic>>;
        using TCommonHandleFileHandler = TItsFileHandler<TAtomic>;
        using TWeightHandleFileHandler = TItsFileHandler<std::atomic<size_t>>;

        TItsFilesWatcher(
            TDisablingFlags& disablingFlags,
            TPanicFlags& panicFlags,
            TDisablingStat& stats,
            TAmnestyFlags& amnestyFlags,
            TServerErrorFlags& serverErrorFlags,
            TSuspiciousFlags& suspiciousFlags,
            TAntirobotDisableExperimentsFlag& antirobotDisableExperimentsFlag,
            std::atomic<size_t>& RSWeight
        );

    public:
        void AddHandler(THolder<IReloadableHandler> handler) {
            const auto loopInterval = ANTIROBOT_DAEMON_CONFIG.HandleWatcherPollInterval;
            Handlers.push_back(std::move(handler));
            Alarmer.Add(loopInterval, 0, true, [handlerPtr = Handlers.back().Get()]() { handlerPtr->ReloadFile(); });
        }

    private:
        TVector<THolder<IReloadableHandler>> Handlers;
        TAlarmer Alarmer;
    };
}
