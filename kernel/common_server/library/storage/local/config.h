#pragma once

#include <kernel/common_server/library/storage/config.h>

namespace NRTProc {
    struct TLocalStorageOptions: public IStorageConfig {
    private:
        static TFactory::TRegistrator<TLocalStorageOptions> Registrator;

    public:
        TFsPath Root = "saas1";

    public:
        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& value);
        NJson::TJsonValue SerializeToJson() const;

        virtual bool Init(const TYandexConfig::Section* section) override;
        virtual void ToString(IOutputStream& os) const override;
        virtual TAtomicSharedPtr<IVersionedStorage> Construct(const TStorageOptions& options) const override;
    };
}
