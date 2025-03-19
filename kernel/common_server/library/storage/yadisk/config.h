#pragma once

#include <kernel/common_server/library/storage/config.h>
#include <kernel/common_server/library/disk/config.h>

namespace NRTProc {
    struct TYaDiskStorageOptions: public IStorageConfig, public TYandexDiskClientConfig {
    private:
        static TFactory::TRegistrator<TYaDiskStorageOptions> Registrator;

    public:
        virtual bool Init(const TYandexConfig::Section* section) override;
        virtual void ToString(IOutputStream& os) const override;
        virtual TAtomicSharedPtr<IVersionedStorage> Construct(const TStorageOptions& options) const override;
    };
}
