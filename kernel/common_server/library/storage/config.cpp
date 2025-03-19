#include "config.h"

#include <library/cpp/json/json_value.h>

#include <util/string/cast.h>
#include <library/cpp/logger/global/global.h>
#include "abstract.h"
#include "cache_storage.h"

namespace NRTProc {
    bool TStorageOptions::Init(const TYandexConfig::Section* section) {
        if (!section)
            return false;
        const TYandexConfig::Directives& directives = section->GetDirectives();
        AssertCorrectConfig(directives.GetValue("Type", Type), "Incorrect storage type: not specified");
        directives.GetValue("ClusterTaskLifetimeSec", ClusterTaskLifetimeSec);
        directives.GetValue("CacheLevel", CacheLevel);

        StorageConfig = IStorageConfig::TFactory::Construct(Type);
        CHECK_WITH_LOG(!!StorageConfig) << "unknown storage type " << Type;
        {
            auto sections = section->GetAllChildren();
            TYandexConfig::TSectionsMap::const_iterator iter = sections.find(Type);
            AssertCorrectConfig(StorageConfig->Init((iter == sections.end()) ? nullptr : iter->second), "Incorrect configuraton for %s", Type.data());
        }
        return true;
    }

    bool TStorageOptions::InitType(const TString& typeName, IStorageConfig::TPtr config) {
        Type = typeName;
        StorageConfig = config;
        CHECK_WITH_LOG(IStorageConfig::TFactory::Has(typeName));
        CHECK_WITH_LOG(!!StorageConfig) << "unknown storage type " << Type << Endl;
        return true;
    }

    bool TStorageOptions::InitType(const TString& typeName, const TYandexConfig::Section* section) {
        Type = typeName;
        StorageConfig = IStorageConfig::TFactory::Construct(Type);
        CHECK_WITH_LOG(!!StorageConfig) << "unknown storage type " << Type << Endl;
        CHECK_WITH_LOG(StorageConfig->Init(section)) << "Incorrect configuraton for " << Type << Endl;
        return true;
    }

    void TStorageOptions::ToString(IOutputStream& os) const {
        os << "Type: " << Type << Endl;
        os << "ClusterTaskLifetimeSec: " << ClusterTaskLifetimeSec << Endl;
        os << "CacheLevel: " << CacheLevel << Endl;
        os << "<" << Type << ">" << Endl;
        StorageConfig->ToString(os);
        os << "</" << Type << ">" << Endl;
    }

    TAtomicSharedPtr<NRTProc::IVersionedStorage> TStorageOptions::ConstructStorage() const {
        CHECK_WITH_LOG(!!StorageConfig);
        IVersionedStorage::TPtr storage = StorageConfig->Construct(*this);
        if (CacheLevel) {
            return MakeAtomicShared<NRTProc::TCachedStorage>(storage);
        }
        return storage;
    }

    TStorageOptions::TStorageOptions()
    {
    }
}
