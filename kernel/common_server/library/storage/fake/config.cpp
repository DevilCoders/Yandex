#include "config.h"
#include "fake_storage.h"

namespace NRTProc {
    bool TFakeStorageOptions::DeserializeFromJson(const NJson::TJsonValue& /*value*/) {
        return true;
    }

    NJson::TJsonValue TFakeStorageOptions::SerializeToJson() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        return result;
    }

    bool TFakeStorageOptions::Init(const TYandexConfig::Section* /*section*/) {
        return true;
    }

    void TFakeStorageOptions::ToString(IOutputStream& /*os*/) const {
    }


    TAtomicSharedPtr<NRTProc::IVersionedStorage> TFakeStorageOptions::Construct(const TStorageOptions& options) const {
        return MakeAtomicShared<TFakeStorage>(options);
    }

    TFakeStorageOptions::TFactory::TRegistrator<TFakeStorageOptions> TFakeStorageOptions::Registrator("Fake");
}
