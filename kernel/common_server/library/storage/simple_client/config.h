#pragma once
#include <kernel/common_server/library/storage/config.h>
#include <kernel/common_server/library/async_impl/async_impl.h>
namespace NRTProc {

    struct TClientStorageOptions: public IStorageConfig, public TAsyncApiImpl::TConfig {
        RTLINE_READONLY_ACCEPTOR(RequestTimeout, TDuration, TDuration::Seconds(1));
    private:
        static TFactory::TRegistrator<TClientStorageOptions> Registrator;
    public:
        virtual bool Init(const TYandexConfig::Section* section) override;
        virtual void ToString(IOutputStream& os) const override;
        virtual TAtomicSharedPtr<IVersionedStorage> Construct(const TStorageOptions& options) const override;
    };


}
