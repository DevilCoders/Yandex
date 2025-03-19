#pragma once

#include "area.h"
#include <kernel/common_server/util/auto_actualization.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <library/cpp/object_factory/object_factory.h>

class IBaseServer;
class IGeoAreasManager;

class IGeoAreasManagerConfig {
public:
    using TPtr = TAtomicSharedPtr<IGeoAreasManagerConfig>;
    using TFactory = NObjectFactory::TObjectFactory<IGeoAreasManagerConfig, TString>;

    virtual ~IGeoAreasManagerConfig() {}
    virtual IGeoAreasManager* BuildManager(const IBaseServer& server) const = 0;
    virtual void Init(const TYandexConfig::Section* section) = 0;
    virtual void ToString(IOutputStream& os) const = 0;
    virtual TString GetClassName() const = 0;
};


class TGeoAreasManagerConfig: public TBaseInterfaceContainer<IGeoAreasManagerConfig> {
private:
    using TBase = TBaseInterfaceContainer<IGeoAreasManagerConfig>;
public:
    using TBase::TBase;
};


class IGeoAreasManager : public IAutoActualization {
protected:
    const IGeoAreasManagerConfig& Config;
public:
    IGeoAreasManager(const IGeoAreasManagerConfig& config)
        : IAutoActualization("GeoAreasManager")
        , Config(config)
    {}

    template <class T>
    const T* GetAs() const {
        return dynamic_cast<const T*>(this);
    }
    template <class T>
    T* GetAs() {
        return dynamic_cast<T*>(this);
    }

    virtual TMaybe<TGeoArea> GetAreaById(const TString& id) const = 0;
    virtual TMaybe<TGeoArea> GetAreaById(const TGeoAreaId& id) const = 0;
    virtual TVector<TGeoArea> GetAreasByType(const TString& type) const = 0;
};
