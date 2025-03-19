#pragma once
#include "storage.h"
#include <kernel/common_server/library/kv/abstract/config.h>

#include <aws/core/Aws.h>

namespace NCS {
    namespace NKVStorage {
        class TS3Config: public IConfig {
        private:
            CSA_READONLY_DEF(TString, Bucket);
            CSA_READONLY_DEF(TString, Region);
            CSA_READONLY_DEF(TString, KeyId);
            CSA_READONLY_DEF(TString, Endpoint);
            CSA_READONLY_DEF(TString, SecretKey);
            CSA_READONLY(TString, CaPath, "/etc/ssl/certs");
            CSA_READONLY(TString, DefaultFolder, "Default");

            Aws::SDKOptions Options;

            static TFactory::TRegistrator<TS3Config> Registrator;
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override {
                Bucket = section->GetDirectives().Value("Bucket", Bucket);
                Region = section->GetDirectives().Value("Region", Region);
                Endpoint = section->GetDirectives().Value("Endpoint", Endpoint);
                KeyId = section->GetDirectives().Value("KeyId", KeyId);
                SecretKey = section->GetDirectives().Value("SecretKey", SecretKey);
                CaPath = section->GetDirectives().Value("CaPath", CaPath);
                DefaultFolder = section->GetDirectives().Value("DefaultFolder", DefaultFolder);
            }
            virtual void DoToString(IOutputStream& os) const override {
                os << "Bucket: " << Bucket << Endl;
                os << "Region: " << Region << Endl;
                os << "Endpoint: " << Endpoint << Endl;
                os << "KeyId: " << KeyId << Endl;
                os << "SecretKey: " << SecretKey << Endl;
                os << "CaPath: " << CaPath << Endl;
                os << "DefaultFolder: " << DefaultFolder << Endl;
            }
            virtual IStorage::TPtr DoBuildStorage() const override;
        public:
            TS3Config() {
                Aws::InitAPI(Options);
            }

            virtual ~TS3Config() override {
                Aws::ShutdownAPI(Options);
            }

            static TString GetTypeName() {
                return "s3";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
