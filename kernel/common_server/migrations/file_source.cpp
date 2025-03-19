#include "config.h"

namespace NCS {
    namespace NStorage {

        class TFileMigrationsSourceConfig: public TDBMigrationsSourceConfig {
        public:
            virtual IDBMigrationsSource::TPtr ConstructSource() const override;
            virtual TString GetClassName() const override {
                return GetTypeName();
            }
            static TString GetTypeName() {
                return "file";
            }

            static TFactory::TRegistrator<TFileMigrationsSourceConfig> Registrar;
            CSA_READONLY_DEF(TFsPath, Path);
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override {
                const auto& dirs = section->GetDirectives();
                dirs.GetValue("Path", Path);
                AssertCorrectConfig(Path.Exists(), "Migrations folder does not exists: %s", Path.GetPath().c_str());
                AssertCorrectConfig(Path.IsFile(), "Migrations folder is not a file: %s", Path.GetPath().c_str());
            }

            virtual void DoToString(IOutputStream& os) const override {
                os << "Path: " << Path << Endl;
            }
        };

        class TFileMigrationsSource : public TDBMigrationsSource {
        public:
            TFileMigrationsSource(const TFileMigrationsSourceConfig& config)
                : TDBMigrationsSource(config)
                , Config(config)
            {}

        protected:
            virtual TVector<TDBMigration> DoCreateMigrations() const override {
                TVector<TDBMigration> result;
                result.emplace_back();
                result.back().ReadFromFile(Config.GetPath(), Config.GetDefaultHeader());
                return result;
            }
        private:
            const TFileMigrationsSourceConfig& Config;
        };

        IDBMigrationsSource::TPtr TFileMigrationsSourceConfig::ConstructSource() const {
            return MakeAtomicShared<TFileMigrationsSource>(*this);
        }

        TDBMigrationsSourceConfig::TFactory::TRegistrator<NCS::NStorage::TFileMigrationsSourceConfig> TFileMigrationsSourceConfig::Registrar(TFileMigrationsSourceConfig::GetTypeName());

    }
}
