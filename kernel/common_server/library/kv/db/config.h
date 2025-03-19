#pragma once
#include "storage.h"
#include <kernel/common_server/library/kv/abstract/config.h>

namespace NCS {
    namespace NKVStorage {
        class TDBConfig: public IConfig {
        private:
            CSA_READONLY_DEF(TString, DBName);
            CSA_READONLY_DEF(TString, DefaultTableName);
            static TFactory::TRegistrator<TDBConfig> Registrator;
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override {
                DBName = section->GetDirectives().Value("DBName", DBName);
                AssertCorrectConfig(!!DBName, "incorrect db name");
                DefaultTableName = section->GetDirectives().Value("DefaultTableName", DefaultTableName);
                AssertCorrectConfig(!!DefaultTableName, "incorrect DefaultTableName");
            }
            virtual void DoToString(IOutputStream& os) const override {
                os << "DBName: " << DBName << Endl;
                os << "DefaultTableName: " << DefaultTableName << Endl;
            }
            virtual IStorage::TPtr DoBuildStorage() const override;
        public:
            static TString GetTypeName() {
                return "db";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
