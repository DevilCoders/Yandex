#include "manager.h"

namespace NCS {
    namespace NStorage {

        void TDBMigrationsSourceConfig::Init(const TYandexConfig::Section* section, const TString& dbName) {
            const TYandexConfig::Directives& dirs = section->GetDirectives();
            Name = section->Name;
            DBName = dirs.Value("DBName", dbName);
            DefaultHeader.SetMaxAttempts(dirs.Value("MaxAttemptsPerMigration", DefaultHeader.GetMaxAttempts()));
            DefaultHeader.SetTimeoutS(dirs.Value("TimeoutPerMigration", TDuration::Seconds(DefaultHeader.GetTimeoutS())).Seconds());
            DefaultHeader.SetRequired(dirs.Value("Required", DefaultHeader.GetRequired()));
            DefaultHeader.SetAutoApply(dirs.Value("AutoApply", DefaultHeader.GetAutoApply()));
            DoInit(section);
        }

        void TDBMigrationsSourceConfig::ToString(IOutputStream& os) const {
            os << "MaxAttemptsPerMigration: " << DefaultHeader.GetMaxAttempts() << Endl;
            os << "TimeoutPerMigration: " << TDuration::Seconds(DefaultHeader.GetTimeoutS()) << Endl;
            os << "Required: " << DefaultHeader.GetRequired() << Endl;
            os << "AutoApply: " << DefaultHeader.GetAutoApply() << Endl;
            os << "DBName: " << DBName << Endl;
            DoToString(os);
        }

        void TDBMigrationsConfig::Init(const TYandexConfig::Section* section, const TString& dbName) {
            TDBEntitiesManagerConfig::Init(section);
            if (!GetDBName()) {
                SetDBName(dbName);
            }
            const auto children = section->GetAllChildren();
            for (auto range = children.equal_range("Sources"); range.first != range.second; ++range.first) {
                for (const auto* curSection = range.first->second->Child; curSection; curSection = curSection->Next) {
                    MigrationSources.emplace_back();
                    MigrationSources.back().Init(curSection, dbName);
                }
            }
        }

        void TDBMigrationsConfig::ToString(IOutputStream& os) const {
            TDBEntitiesManagerConfig::ToString(os);
            for (const auto& source: MigrationSources) {
                os << "<" << source->GetName() << ">" << Endl;
                source.ToString(os);
                os << "</" << source->GetName() << ">" << Endl;
            }
        }

        TDBMigrationsSource::TDBMigrationsSource(const TDBMigrationsSourceConfig& config)
            : Config(config)
        {}

        TVector<TDBMigration> TDBMigrationsSource::CreateMigrations() const {
            auto result = DoCreateMigrations();
            for (auto& m: result) {
                m.SetSource(Config.GetName());
            }
            return result;
        }
    }
}
