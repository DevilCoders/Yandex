#include "config.h"

namespace NCS {
    namespace NStorage {
        class TFolderMigrationsSource;

        class TFolderMigrationsSourceConfig: public TDBMigrationsSourceConfig {
        public:
            virtual IDBMigrationsSource::TPtr ConstructSource() const override;
            virtual TString GetClassName() const override {
                return GetTypeName();
            }
            static TString GetTypeName() {
                return "folder";
            }
            static TFactory::TRegistrator<TFolderMigrationsSourceConfig> Registrar;
            CSA_READONLY_DEF(TFsPath, Path);

        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override {
                const auto& dirs = section->GetDirectives();
                dirs.GetValue("Path", Path);
                AssertCorrectConfig(Path.Exists(), "Migrations folder does not exists: %s", Path.GetPath().c_str());
                AssertCorrectConfig(Path.IsDirectory(), "Migrations folder is not a folder: %s", Path.GetPath().c_str());
            }

            virtual void DoToString(IOutputStream& os) const override {
                os << "Path: " << Path << Endl;
            }
        };

        class TFolderMigrationsSource : public TDBMigrationsSource {
        public:
            TFolderMigrationsSource(const TFolderMigrationsSourceConfig& config)
                : TDBMigrationsSource(config)
                , Config(config)
            {}

            virtual TVector<TDBMigration> DoCreateMigrations() const override {
                TVector<TDBMigration> result;
                TVector<TString> files;
                Config.GetPath().ListNames(files);
                Sort(files);
                for (const auto& name : files) {
                    result.emplace_back();
                    result.back().ReadFromFile(Config.GetPath() / name, Config.GetDefaultHeader());
                }
                return result;
            }
        private:
            const TFolderMigrationsSourceConfig& Config;
        };

        IDBMigrationsSource::TPtr TFolderMigrationsSourceConfig::ConstructSource() const {
            return MakeAtomicShared<TFolderMigrationsSource>(*this);
        }

        TDBMigrationsSourceConfig::TFactory::TRegistrator<NCS::NStorage::TFolderMigrationsSourceConfig> TFolderMigrationsSourceConfig::Registrar(TFolderMigrationsSourceConfig::GetTypeName());

    }
}
