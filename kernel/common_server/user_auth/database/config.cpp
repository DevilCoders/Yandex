#include "config.h"
#include "manager.h"

TDBAuthUsersManagerConfig::TFactory::TRegistrator<TDBAuthUsersManagerConfig> TDBAuthUsersManagerConfig::Registrator(TDBAuthUsersManagerConfig::GetTypeName());

IAuthUsersManager::TPtr TDBAuthUsersManagerConfig::BuildManager(const IBaseServer& server) const {
    THolder<IHistoryContext> historyContext;
    auto db = server.GetDatabase(DBName);
    CHECK_WITH_LOG(!!db) << "Incorrect db name: " << DBName;
    historyContext = MakeHolder<THistoryContext>(db);
    return MakeAtomicShared<TDBAuthUsersManager>(std::move(historyContext), *this, server);
}

void TDBAuthUsersManagerConfig::Init(const TYandexConfig::Section* section) {
    auto children = section->GetAllChildren();
    auto it = children.find("HistoryConfig");
    if (it != children.end()) {
        HistoryConfig.Init(it->second);
    }
    DBName = section->GetDirectives().Value("DBName", DBName);
    AssertCorrectConfig(!!DBName, "Incorrect DBName for UsersManagerConfig");
    UseAuthModuleId = section->GetDirectives().Value("UseAuthModuleId", UseAuthModuleId);
}

void TDBAuthUsersManagerConfig::ToString(IOutputStream& os) const {
    os << "<HistoryConfig>" << Endl;
    HistoryConfig.ToString(os);
    os << "</HistoryConfig>" << Endl;
    os << "DBName: " << DBName << Endl;
    os << "UseAuthModuleId: " << UseAuthModuleId << Endl;
}
