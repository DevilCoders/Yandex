#include "config.h"
#include "storage.h"

namespace NRTProc {
    bool TClientStorageOptions::Init(const TYandexConfig::Section* section) {
        RequestTimeout = section->GetDirectives().Value<TDuration>("RequestTimeout", RequestTimeout);
        TAsyncApiImpl::TConfig::Init(section, nullptr);
        return true;
    }

    void TClientStorageOptions::ToString(IOutputStream& os) const {
        TAsyncApiImpl::TConfig::ToString(os);
        os << "RequestTimeout: " << RequestTimeout << Endl;
    }

    TAtomicSharedPtr<NRTProc::IVersionedStorage> TClientStorageOptions::Construct(const TStorageOptions& options) const {
        return new TClientStorage(options, *this);
    }

    TClientStorageOptions::TFactory::TRegistrator<TClientStorageOptions> TClientStorageOptions::Registrator("simple_client");
}
