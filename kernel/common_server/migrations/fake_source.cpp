#include "config.h"

namespace NCS {
    namespace NStorage {

        class TFakeMigrationsSource: public TDBMigrationsSource {
        public:
            using TDBMigrationsSource::TDBMigrationsSource;

        protected:
            virtual TVector<TDBMigration> DoCreateMigrations() const override {
                return {};
            }
        
        };

        class TFakeMigrationsSourceConfig: public TDBMigrationsSourceConfig {
        public:
            virtual IDBMigrationsSource::TPtr ConstructSource() const override {
                return MakeAtomicShared<TFakeMigrationsSource>(*this);
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            static TString GetTypeName() {
                return "fake";
            }

        protected:
            virtual void DoInit(const TYandexConfig::Section* /*section*/) override {
            }

            virtual void DoToString(IOutputStream& /*os*/) const override {
            }

            static TFactory::TRegistrator<TFakeMigrationsSourceConfig> Registrar;
        };

        TDBMigrationsSourceConfig::TFactory::TRegistrator<NCS::NStorage::TFakeMigrationsSourceConfig> TFakeMigrationsSourceConfig::Registrar(TFakeMigrationsSourceConfig::GetTypeName());

    }
}
