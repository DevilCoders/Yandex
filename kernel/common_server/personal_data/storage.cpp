#include "storage.h"

IPersonalDataStorage::TFactory::TRegistrator<TFakePersonalDataStorage> TFakePersonalDataStorage::Registrator("fake");

IPersonalDataStorage* TPersonalDataStorageConfig::BuildStorage(IBaseServer& server) const {
    auto storage = IPersonalDataStorage::TFactory::Construct(Type, *this, server);
    AssertCorrectConfig(!!storage,  "Unknown PersonalDataStorage %s", Type.data());
    return storage;
}
