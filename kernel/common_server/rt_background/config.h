#pragma once
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/yconf/conf.h>
#include <util/stream/output.h>
#include <util/datetime/base.h>
#include <kernel/common_server/api/history/config.h>
#include "storages/abstract.h"
#include "tasks/abstract/manager.h"

class IBaseServer;
class IRTBackgroundProcessesStorage;

class TRTBackgroundManagerConfig {
    CSA_READONLY(ui32, ThreadsCount, 16);
    CSA_READONLY_DEF(TString, DBName);
    CSA_READONLY_DEF(NBServer::TRTBackgroundStorageConfigContainer, StorageConfig);
    CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
    CSA_READONLY(TDuration, PingPeriod, TDuration::Seconds(1));
    CSA_READONLY(TDuration, FinishAttemptionsTimeout, TDuration::Minutes(10));
    CSA_READONLY(TDuration, FinishAttemptionsPause, TDuration::Seconds(5));
    CSA_READONLY_DEF(TVector<NCS::NBackground::TTasksManagerConfigContainer>, TaskManagers);
public:
    TRTBackgroundManagerConfig() = default;
    void Init(const TYandexConfig::Section* section);

    void ToString(IOutputStream& os) const;
};

