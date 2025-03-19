#include "config.h"
#include "storage.h"

namespace NRTProc {
    bool TYaDiskStorageOptions::Init(const TYandexConfig::Section* section) {
        TYandexDiskClientConfig::Init(section, nullptr);
        return true;
    }

    void TYaDiskStorageOptions::ToString(IOutputStream& os) const {
        TYandexDiskClientConfig::ToString(os);
    }

    TAtomicSharedPtr<NRTProc::IVersionedStorage> TYaDiskStorageOptions::Construct(const TStorageOptions& options) const {
        return new TYaDiskStorage(options, *this);
    }

    TClientStorageOptions::TFactory::TRegistrator<TYaDiskStorageOptions> TYaDiskStorageOptions::Registrator("yadisk_client");
}
