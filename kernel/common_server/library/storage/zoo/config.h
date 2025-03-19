#pragma once
#include <kernel/common_server/library/storage/config.h>

namespace NRTProc {

    class TZooStorageOptions: public IStorageConfig {
    private:
        static TFactory::TRegistrator<TZooStorageOptions> Registrator;
        TString Root = "saas1";
    public:
        TString Address;
        ui32 CacheSize = 10000;
        ui32 LogLevel = 3; // LL_INFO

        TString GetRoot() const {
            return TFsPath("/" + Root).Fix().GetPath();
        }

        TZooStorageOptions& SetRoot(const TString& value) {
            Root = value;
            return *this;
        }

        TZooStorageOptions& SetAddress(const TString& value) {
            Address = value;
            return *this;
        }

        TZooStorageOptions& SetLogLevel(const ui32 value) {
            LogLevel = value;
            return *this;
        }


        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& value);
        NJson::TJsonValue SerializeToJson() const;
        virtual bool Init(const TYandexConfig::Section* section) override;
        virtual void ToString(IOutputStream& os) const override;
        virtual TAtomicSharedPtr<IVersionedStorage> Construct(const TStorageOptions& options) const override;
    };

}
