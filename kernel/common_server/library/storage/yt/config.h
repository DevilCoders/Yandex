#pragma once
#include <kernel/common_server/library/storage/config.h>

namespace NRTProc {
    struct TYTOptions: public IStorageConfig {
    private:
        TString ServerName = "hahn";
        TString RootPath = "RTLINE";
        static TFactory::TRegistrator<TYTOptions> Registrator;
    public:
        static const TString ClassName;

        const TString& GetServerName() const {
            return ServerName;
        }

        const TString& GetRootPath() const {
            return RootPath;
        }

        TYTOptions() {

        }

        virtual TAtomicSharedPtr<IVersionedStorage> Construct(const TStorageOptions& options) const override;

        NJson::TJsonValue SerializeToJson() const;
        bool DeserializeFromJson(const NJson::TJsonValue& value);

        virtual bool Init(const TYandexConfig::Section* section) override {
            if (!section)
                return false;
            if (!section->GetDirectives().GetValue("ServerName", ServerName)) {
                return false;
            }
            if (!section->GetDirectives().GetValue("RootPath", RootPath)) {
                return false;
            }
            return true;
        }

        virtual void ToString(IOutputStream& os) const override {
            os << "ServerName: " << ServerName << Endl;
            os << "RootPath: " << RootPath << Endl;
        }
    };

}
