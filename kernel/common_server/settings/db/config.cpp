#include "config.h"
#include "settings.h"

namespace NCS {
    TDBSettingsConfig::TFactory::TRegistrator<TDBSettingsConfig> TDBSettingsConfig::Registrator(TDBSettingsConfig::GetTypeName());

    void TDBSettingsConfig::DoInit(const TYandexConfig::Section& section) {
        Prefix = section.GetDirectives().Value("Prefix", Prefix);
        DBName = section.GetDirectives().Value("DBName", DBName);
        AssertCorrectConfig(!!DBName, "Incorrect DBName for settings");
        HistoryConfig.Init(&section);
    }

    void TDBSettingsConfig::DoToString(IOutputStream& os) const {
        os << "DBName: " << DBName << Endl;
        os << "Prefix: " << Prefix << Endl;
        HistoryConfig.ToString(os);
    }

    ISettings::TPtr TDBSettingsConfig::Construct(const IBaseServer& server) const {
        NStorage::IDatabase::TPtr db = server.GetDatabase(DBName);
        AssertCorrectConfig(!!db, "Incorrect db for settings %s", DBName.data());
        auto hc = MakeHolder<THistoryContext>(db);
        return MakeAtomicShared<TSettingsDB>(std::move(hc), *this);
    }

}
