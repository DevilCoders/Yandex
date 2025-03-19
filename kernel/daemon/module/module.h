#pragma once

#include <kernel/daemon/config/daemon_config.h>
#include <kernel/daemon/config/config_constructor.h>

#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/logger/global/global.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/cast.h>
#include <util/generic/noncopyable.h>
#include <util/generic/maybe.h>

class IServerConfig;

class IAbstractModuleConfig {
public:
    typedef TAtomicSharedPtr<IAbstractModuleConfig> TPtr;

public:
    virtual ~IAbstractModuleConfig() {}
    virtual bool Init(const IServerConfig& serverConfig, const TYandexConfig::Section* componentsSection, const TString& componentName) = 0;
    virtual bool Check() const = 0;
    virtual void ToString(IOutputStream& so) const = 0;
    virtual bool ConfigurationExists() const = 0;
};

class IDaemonModuleConfig: public IAbstractModuleConfig {
public:
    typedef NObjectFactory::TObjectFactory<IDaemonModuleConfig, TString> TFactory;
};

class IServerConfig: public TNonCopyable {
private:
    virtual void OnAfterCreate(const TServerConfigConstructorParams& /*params*/) {

    }

public:

    template <class TConfigConstructed>
    static THolder<IServerConfig> BuildConfig(const TServerConfigConstructorParams& params) {
        THolder<IServerConfig> result(new TConfigConstructed(params));
        result->OnAfterCreate(params);
        return result;
    }

    const TString& GetCType() const {
        return GetDaemonConfig().GetCType();
    }
    const TString& GetService() const {
        return GetDaemonConfig().GetService();
    }
    virtual ~IServerConfig() {}
    virtual TSet<TString> GetModulesSet() const = 0;
    virtual const TDaemonConfig& GetDaemonConfig() const = 0;

    template<class T>
    const T& GetMeAs() const {
        return *VerifyDynamicCast<const T*>(this);
    }
    template <class T>
    T& GetMeAs() {
        return *VerifyDynamicCast<T*>(this);
    }
    template <class T>
    T* SafeGetMeAs() {
        return dynamic_cast<T*>(this);
    }
    template <class T>
    const T* SafeGetMeAs() const {
        return dynamic_cast<const T*>(this);
    }
    template <class T>
    const T* GetModuleConfig(const TString& name) const {
        return VerifyDynamicCast<const T*>(GetModuleConfigImpl(name));
    }

protected:
    virtual const IAbstractModuleConfig* GetModuleConfigImpl(const TString& /*name*/) const {
        return nullptr;
    }
};

template <class TBaseClass>
class TPluginConfig: public TBaseClass {
private:
    bool ConstructedWithConfiguration = false;

public:
    virtual bool ConfigurationExists() const override {
        return ConstructedWithConfiguration;
    }

    virtual bool Check() const override {
        const bool result = DoCheck();
        INFO_LOG << ComponentName << " config check result: " << (result ? "OK" : "FAILED") << Endl;
        return result;
    }

    virtual bool Init(const IServerConfig& serverConfig, const TYandexConfig::Section* componentsSection, const TString& componentName) override {
        Owner = &serverConfig;
        ComponentName = componentName;
        if (componentsSection) {
            const TYandexConfig::TSectionsMap children = componentsSection->GetAllChildren();
            const TYandexConfig::TSectionsMap::const_iterator iter = children.find(ComponentName);
            if (iter != children.end()) {
                ConstructedWithConfiguration = true;
                DoInit(*iter->second);
                return true;
            }
        }
        return false;
    }

    const IServerConfig& GetOwner() const {
        CHECK_WITH_LOG(Owner);
        return *Owner;
    }

    virtual void ToString(IOutputStream& so) const override {
        if (ConstructedWithConfiguration) {
            so << "<" << ComponentName << ">" << Endl;
            DoToString(so);
            so << "</" << ComponentName << ">" << Endl;
        }
    }

    TString ComponentName;

protected:
    virtual bool DoCheck() const = 0;
    virtual void DoInit(const TYandexConfig::Section& componentSection) = 0;
    virtual void DoToString(IOutputStream& so) const = 0;

protected:
    const IServerConfig* Owner;
};

template <class IConfig>
class TPluginConfigsBase {
public:
    typedef THashMap<TString, typename IConfig::TPtr> TConfigHashMap;

public:
    virtual ~TPluginConfigsBase() {}

    template <class T>
    const T* Get(const TString& pluginName) const {
        typename TConfigHashMap::const_iterator i = ConfigHashMap.find(pluginName);
        return i == ConfigHashMap.end() ? nullptr : VerifyDynamicCast<const T*>(i->second.Get());
    }
    bool Check() const {
        for (typename TConfigHashMap::const_iterator i = ConfigHashMap.begin(); i != ConfigHashMap.end(); ++i)
            if (i->second->ConfigurationExists() && !i->second->Check())
                return false;
        return true;
    }

    void ToString(IOutputStream* so) const {
        *so << "<" << Name << ">" << Endl;
        for (typename TConfigHashMap::const_iterator i = ConfigHashMap.begin(); i != ConfigHashMap.end(); ++i)
            i->second->ToString(*so);
        *so << "</" << Name << ">" << Endl;
    }

    void Init(const IServerConfig& serverConfig, const TYandexConfig::TSectionsMap& sections) {
        ConfigHashMap.clear();
        TYandexConfig::TSectionsMap::const_iterator iter = sections.find(Name);
        const TYandexConfig::Section* ccSection = iter == sections.end() ? nullptr : iter->second;
        InitFromSection(serverConfig, ccSection);
    }

    TSet<TString> GetModules() const {
        TSet<TString> result;
        for (auto i : ConfigHashMap) {
            result.insert(i.first);
        }
        return result;
    }

protected:
    TPluginConfigsBase(const TString& name)
        : Name(name)
    {}

    virtual void InitFromSection(const IServerConfig& serverConfig, const TYandexConfig::Section* ccSection) = 0;

protected:
    TConfigHashMap ConfigHashMap;
    const TString Name;
};

template <class IConfig>
class TPluginConfigs : public TPluginConfigsBase<IConfig> {
public:
    typedef TPluginConfigsBase<IConfig> TBase;

public:
    TPluginConfigs(const IServerConfig& serverConfig, const TString& name)
        : TBase(name)
    {
        InitFromSection(serverConfig, nullptr);
    }

protected:
    virtual void InitFromSection(const IServerConfig& serverConfig, const TYandexConfig::Section* ccSection) override {
        TSet<TString> componentConfigsNames;
        Singleton<typename IConfig::TFactory>()->GetKeys(componentConfigsNames);
        for (TSet<TString>::const_iterator i = componentConfigsNames.begin(); i != componentConfigsNames.end(); ++i) {
            INFO_LOG << TBase::Name << " section reading... " << *i << Endl;
            VERIFY_WITH_LOG(IConfig::TFactory::Has(*i), "can't find registered config for %s", i->c_str());
            typename IConfig::TPtr cc(IConfig::TFactory::Construct(*i));
            VERIFY_WITH_LOG(!!cc, "cannot construct config for %s", i->c_str());
            cc->Init(serverConfig, ccSection, *i);
            INFO_LOG << TBase::Name << " section saved  " << *i << Endl;
            TBase::ConfigHashMap[*i] = cc;
            INFO_LOG << TBase::Name << " section reading... " << *i << " OK" << Endl;
        }
    }
};

template <class IConfig>
class TTypedPluginConfigs: public TPluginConfigsBase<IConfig> {
public:
    typedef TPluginConfigsBase<IConfig> TBase;

public:
    TTypedPluginConfigs(const TString& name)
        : TBase(name)
    {
    }

protected:
    virtual void InitFromSection(const IServerConfig& serverConfig, const TYandexConfig::Section* ccSection) override {
        if (!ccSection)
            return;

        TYandexConfig::TSectionsMap children = ccSection->GetAllChildren();
        for (const auto& child : children) {
            INFO_LOG << TBase::Name << " section reading... " << child.first << Endl;
            TString type;
            if (!child.second->GetDirectives().GetValue("Type", type))
                ythrow yexception() << "there is no Type for " << child.first;

            VERIFY_WITH_LOG(IConfig::TFactory::Has(type), "can't find registered config for %s", type.c_str());
            typename IConfig::TPtr cc(IConfig::TFactory::Construct(type, type));
            VERIFY_WITH_LOG(!!cc, "cannot construct config for %s", type.data());
            cc->Init(serverConfig, ccSection, child.first);
            INFO_LOG << TBase::Name << " section saved  " << child.first << Endl;
            TBase::ConfigHashMap[child.first] = cc;
            INFO_LOG << TBase::Name << " section reading... " << child.first << " OK" << Endl;
        }
    }
};

class IDaemonModule {
public:
    struct TStopContext {
        ui32 RigidLevel = 0;
    };

    typedef NObjectFactory::TParametrizedObjectFactory<IDaemonModule, TString, const TDaemonConfig&> TFactory;

public:
    virtual ~IDaemonModule() {}

    virtual bool Start() = 0;
    virtual bool Stop(const TStopContext& stopContext) = 0;
    virtual TString Name() const = 0;
};

class TDaemonModules {
private:
    const IServerConfig& Config;
    TVector<THolder<IDaemonModule>> Modules;

public:
    typedef NObjectFactory::TParametrizedObjectFactory<IDaemonModule, TString, const IServerConfig&> TFactory;

public:
    TDaemonModules(const IServerConfig& config);

    bool Start();
    bool Stop(const IDaemonModule::TStopContext& stopContext);
};
