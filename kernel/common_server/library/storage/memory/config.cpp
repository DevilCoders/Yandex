#include "config.h"
#include "mem_storage.h"

namespace NRTProc {
    bool TMemoryStorageOptions::DeserializeFromJson(const NJson::TJsonValue& /*value*/) {
        return true;
    }

    NJson::TJsonValue TMemoryStorageOptions::SerializeToJson() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        return result;
    }

    bool TMemoryStorageOptions::Init(const TYandexConfig::Section* /*section*/) {
        return true;
    }

    void TMemoryStorageOptions::ToString(IOutputStream& /*os*/) const {
    }

    TAtomicSharedPtr<NRTProc::IVersionedStorage> TMemoryStorageOptions::Construct(const TStorageOptions& options) const {
        return new TMemoryStorage(options, *this);
    }

    TMemoryStorageOptions::TFactory::TRegistrator<TMemoryStorageOptions> TMemoryStorageOptions::Registrator("Memory");
}
