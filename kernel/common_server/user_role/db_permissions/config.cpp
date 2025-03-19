#include "config.h"
#include "manager.h"

TDBPermissionsManagerConfig::TFactory::TRegistrator<TDBPermissionsManagerConfig> TDBPermissionsManagerConfig::Registrator(TDBPermissionsManagerConfig::GetTypeName());

void TDBPermissionsManagerConfig::DoToString(IOutputStream& os) const {
    os << "DBName: " << DBName << Endl;
    os << "<HistoryConfig>" << Endl;
    HistoryConfig.ToString(os);
    os << "</HistoryConfig>" << Endl;
}

void TDBPermissionsManagerConfig::DoInit(const TYandexConfig::Section* section) {
    DBName = section->GetDirectives().Value("DBName", DBName);
    auto children = section->GetAllChildren();
    {
        auto it = children.find("HistoryConfig");
        if (it != children.end()) {
            HistoryConfig.Init(it->second);
        }
    }
}

THolder<IPermissionsManager> TDBPermissionsManagerConfig::BuildManager(const IBaseServer& server) const {
    return MakeHolder<TDBPermissionsManager>(DBName, HistoryConfig, *this, server);
}
