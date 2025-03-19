#include "config.h"
#include <library/cpp/mediator/global_notifications/system_status.h>

void TRTBackgroundManagerConfig::Init(const TYandexConfig::Section* section) {
    ThreadsCount = section->GetDirectives().Value("ThreadsCount", ThreadsCount);
    DBName = section->GetDirectives().Value("DBName", DBName);
    AssertCorrectConfig(!!DBName, "Incorrect DBName for rt background manager");
    PingPeriod = section->GetDirectives().Value("PingPeriod", PingPeriod);
    FinishAttemptionsTimeout = section->GetDirectives().Value("FinishAttemptionsTimeout", FinishAttemptionsTimeout);
    FinishAttemptionsPause = section->GetDirectives().Value("FinishAttemptionsPause", FinishAttemptionsPause);
    StorageConfig.Init(section);
    HistoryConfig.Init(section);

    const auto children = section->GetAllChildren();
    auto it = children.find("TaskManagers");
    if (it != children.end()) {
        const auto tmChildren = it->second->GetAllChildren();
        for (auto&& i : tmChildren) {
            NCS::NBackground::TTasksManagerConfigContainer container;
            container.Init(i.second);
            TaskManagers.emplace_back(std::move(container));
        }
    }
}

void TRTBackgroundManagerConfig::ToString(IOutputStream& os) const {
    os << "ThreadsCount: " << ThreadsCount << Endl;
    os << "DBName: " << DBName << Endl;
    os << "PingPeriod: " << PingPeriod << Endl;
    os << "FinishAttemptionsTimeout: " << FinishAttemptionsTimeout << Endl;
    os << "FinishAttemptionsPause: " << FinishAttemptionsPause << Endl;
    StorageConfig.ToString(os);
    HistoryConfig.ToString(os);
    os << "<TaskManagers>" << Endl;
    for (auto&& i : TaskManagers) {
        i.ToString(os);
    }
    os << "</TaskManagers>" << Endl;
}
