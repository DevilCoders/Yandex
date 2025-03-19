#pragma once

#include <library/cpp/yconf/conf.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <util/datetime/base.h>
#include <util/folder/path.h>
#include <util/generic/string.h>
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/util/accessor.h>

namespace NJson {
    class TJsonValue;
}

namespace NRTProc {

    class IVersionedStorage;
    class TStorageOptions;

    class IStorageConfig {
    public:
        using TPtr = TAtomicSharedPtr<IStorageConfig>;
        using TFactory = NObjectFactory::TParametrizedObjectFactory<IStorageConfig, TString>;
        virtual bool Init(const TYandexConfig::Section* section) = 0;
        virtual void ToString(IOutputStream& os) const = 0;
        virtual TAtomicSharedPtr<IVersionedStorage> Construct(const TStorageOptions& options) const = 0;
        virtual ~IStorageConfig() {

        }
    };

    class TStorageOptions {
        RTLINE_READONLY_ACCEPTOR(Type, TString, "ZOO");
        RTLINE_READONLY_ACCEPTOR(CacheLevel, ui32, 0);
    private:
        IStorageConfig::TPtr StorageConfig;
    public:
        IStorageConfig::TPtr GetStorageConfig() const {
            return StorageConfig;
        }
        ui64 ClusterTaskLifetimeSec = 0;

        TStorageOptions();

        bool Init(const TYandexConfig::Section* section);
        bool InitType(const TString& typeName, const TYandexConfig::Section* section);
        bool InitType(const TString& typeName, IStorageConfig::TPtr config);
        void ToString(IOutputStream& os) const;
        TAtomicSharedPtr<IVersionedStorage> ConstructStorage() const;
    };

};
