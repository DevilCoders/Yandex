#include "its_files_watcher.h"

#include "config_global.h"

namespace NAntiRobot {
    namespace {
        std::function<void(EHostType, bool)> GetPanicNeverBanStatusCallback(TDisablingStat& stats) {
            return [&stats](NAntiRobot::EHostType service, bool status) {
                if (status) {
                    stats.EnablePanicNeverBan(service);
                } else {
                    stats.DisablePanicNeverBan(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetPanicNeverBlockStatusCallback(TDisablingStat& stats) {
            return [&stats](NAntiRobot::EHostType service, bool status) {
                if (status) {
                    stats.EnablePanicNeverBlock(service);
                } else {
                    stats.DisablePanicNeverBlock(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetPanicMainStatusCallback(TDisablingStat& stats) {
            return [&stats](NAntiRobot::EHostType service, bool status) {
                if (status) {
                    stats.EnablePanicMainOnly(service);
                } else {
                    stats.DisablePanicMainOnly(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetPanicDzensearchStatusCallback(TDisablingStat& stats) {
            return [&stats](NAntiRobot::EHostType service, bool status) {
                if (status) {
                    stats.EnablePanicDzensearchOnly(service);
                } else {
                    stats.DisablePanicDzensearchOnly(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetPanicBanAllStatusCallback(TDisablingStat& stats) {
            return [&stats](NAntiRobot::EHostType service, bool status) {
                if (status) {
                    stats.EnablePanicMayBanFor(service);
                } else {
                    stats.DisablePanicMayBanFor(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetPanicShowCaptchaAllStatusCallback(TDisablingStat& stats) {
            return [&stats](NAntiRobot::EHostType service, bool status) {
                if (status) {
                    stats.EnablePanicCanShowCaptcha(service);
                } else {
                    stats.DisablePanicCanShowCaptcha(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetPanicPreviewIdentTypeEnabledStatusCallback(TDisablingStat& stats) {
            return [&stats](NAntiRobot::EHostType service, bool status) {
                if (status) {
                    stats.EnablePanicPreviewIdentTypeEnabled(service);
                } else {
                    stats.DisablePanicPreviewIdentTypeEnabled(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetAmnestyCallback(TAmnestyFlags& flags) {
            return [&flags](EHostType service, bool value) {
                flags.SetAmnesty(service, value);
            };
        }

        std::function<void(EHostType, bool)> GetErrorServiceDisableCallback(TServerErrorFlags& flags) {
            return [&flags](EHostType service, bool status) {
                if (status) {
                    flags.EnableErrorServiceDisable(service);
                } else {
                    flags.DisableErrorServiceDisable(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetPanicModeCbbCallback(TDisablingStat& stats) {
            return [&stats](EHostType service, bool status) {
                if (status) {
                    stats.EnablePanicCbb(service);
                } else {
                    stats.DisablePanicCbb(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetManyRequestsStatusCallback(TSuspiciousFlags& flags) {
            return [&flags](EHostType service, bool status) {
                if (status) {
                    flags.EnableManyRequests(service);
                } else {
                    flags.DisableManyRequests(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetManyRequestsMobileStatusCallback(TSuspiciousFlags& flags) {
            return [&flags](EHostType service, bool status) {
                if (status) {
                    flags.EnableManyRequestsMobile(service);
                } else {
                    flags.DisableManyRequestsMobile(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetSuspiciousBanCallback(TSuspiciousFlags& flags) {
            return [&flags](EHostType service, bool status) {
                if (status) {
                    flags.EnableBan(service);
                } else {
                    flags.DisableBan(service);
                }
            };
        }

        std::function<void(EHostType, bool)> GetSuspiciousBlockCallback(TSuspiciousFlags& flags) {
            return [&flags](EHostType service, bool status) {
                if (status) {
                    flags.EnableBlock(service);
                } else {
                    flags.DisableBlock(service);
                }
            };
        }

    }  // namespace

    TItsFilesWatcher::TItsFilesWatcher(
        TDisablingFlags& disablingFlags,
        TPanicFlags& panicFlags,
        TDisablingStat& stats,
        TAmnestyFlags& amnestyFlags,
        TServerErrorFlags& serverErrorFlags,
        TSuspiciousFlags& suspiciousFlags,
        TAntirobotDisableExperimentsFlag& antirobotDisableExperimentsFlag,
        std::atomic<size_t>& RSWeight
    ) {
        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopBlockFilePath,
            disablingFlags.NeverBlockByService,
            GetPanicNeverBlockStatusCallback(stats)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopBanFilePath,
            disablingFlags.NeverBanByService,
            GetPanicNeverBanStatusCallback(stats)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleCatboostWhitelistServicePath,
            disablingFlags.DisableCatbostWhitelist
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleAllowMainBanFilePath,
            *panicFlags.PanicMainOnlyByService,
            GetPanicMainStatusCallback(stats)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleAllowDzenSearchBanFilePath,
            *panicFlags.PanicDzensearchByService,
            GetPanicDzensearchStatusCallback(stats)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleAllowBanAllFilePath,
            *panicFlags.PanicMayBanByService,
            GetPanicBanAllStatusCallback(stats)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleAllowShowCaptchaAllFilePath,
            *panicFlags.PanicCanShowCaptchaByService,
            GetPanicShowCaptchaAllStatusCallback(stats)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleAmnestyFilePath,
            amnestyFlags.ByService,
            GetAmnestyCallback(amnestyFlags)
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopBlockForAllFilePath,
            disablingFlags.NeverBlock
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopBanForAllFilePath,
            disablingFlags.NeverBan
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleAmnestyForAllFilePath,
            amnestyFlags.ForAll,
            GetAmnestyCallback(amnestyFlags)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandlePreviewIdentTypeEnabledFilePath,
            *panicFlags.PanicPreviewIdentTypeEnabledByService,
            GetPanicPreviewIdentTypeEnabledStatusCallback(stats)
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopFuryForAllFilePath,
            disablingFlags.StopFury
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopFuryPreprodForAllFilePath,
            disablingFlags.StopFuryPreprod
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopYqlFilePath,
            disablingFlags.StopYql
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopYqlForAllFilePath,
            disablingFlags.StopYqlForAll
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleStopDiscoveryForAllFilePath,
            disablingFlags.StopDiscoveryForAll
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleCatboostWhitelistAllFilePath,
            disablingFlags.DisableCatboostWhitelistForAll
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleServerErrorDisableServicePath,
            serverErrorFlags.ServiceDisable,
            GetErrorServiceDisableCallback(serverErrorFlags)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleCbbPanicModePath,
            *panicFlags.PanicCbbByService,
            GetPanicModeCbbCallback(stats)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleManyRequestsEnableServicePath,
            suspiciousFlags.ManyRequests,
            GetManyRequestsStatusCallback(suspiciousFlags)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleManyRequestsMobileEnableServicePath,
            suspiciousFlags.ManyRequestsMobile,
            GetManyRequestsMobileStatusCallback(suspiciousFlags)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleSuspiciousBanServicePath,
            suspiciousFlags.Ban,
            GetSuspiciousBanCallback(suspiciousFlags)
        ));

        AddHandler(MakeHolder<TServiceHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleSuspiciousBlockServicePath,
            suspiciousFlags.Block,
            GetSuspiciousBlockCallback(suspiciousFlags)
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleServerErrorEnablePath,
            serverErrorFlags.Enable
        ));

        AddHandler(MakeHolder<TCommonHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandleAntirobotDisableExperimentsPath,
            antirobotDisableExperimentsFlag.Enable
        ));

        AddHandler(MakeHolder<TWeightHandleFileHandler>(
            ANTIROBOT_DAEMON_CONFIG.HandlePingControlFilePath,
            RSWeight
        ));
    }
}
