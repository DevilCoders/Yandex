#pragma once
#include <kernel/daemon/config/daemon_config.h>
#include <kernel/daemon/module/module.h>
#include <library/cpp/yconf/conf.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <kernel/daemon/config/config_constructor.h>
#include <util/system/getpid.h>

class TConfig: public IServerConfig {
private:
    TDaemonConfig DaemonConfig;
    TString State = "OK";
    TPluginConfigs<IDaemonModuleConfig> ModulesConfig;
    TSet<TString> UsingModules;

    void Init(const TYandexConfig::Section* section) {
        CHECK_WITH_LOG(section);
        auto sections = section->GetAllChildren();
        auto it = sections.find("Server");
        CHECK_WITH_LOG(it != sections.end());
        const TYandexConfig::Directives& dirs = it->second->GetDirectives();
        AssertCorrectConfig(dirs.GetValue("State", State), "Incorrect state");
        AssertCorrectConfig(State != "FAIL", "Incorrect state");
        Y_ENSURE(State != "FAIL_EXCEPTION");
        CHECK_WITH_LOG(State != "INCORRECT_FAIL");

        ModulesConfig.Init(*this, sections);
        auto modules = sections.find("ModulesConfig");
        if (modules != sections.end()) {
            for (auto&& module : modules->second->GetAllChildren()) {
                auto directives = module.second->GetDirectives();
                bool enabled = false;
                CHECK_WITH_LOG(directives.find("Enabled") != directives.end() && directives.GetValue("Enabled", enabled));
                if (enabled) {
                    CHECK_WITH_LOG(IDaemonModuleConfig::TFactory::Has(module.first));
                    UsingModules.insert(module.first);
                }
            }
        }

    }

    void InitFromString(const TString& str) {
        TAnyYandexConfig config;
        CHECK_WITH_LOG(config.ParseMemory(str));
        Init(config.GetRootSection());
    }

    virtual const IAbstractModuleConfig* GetModuleConfigImpl(const TString& moduleName) const override {
        return ModulesConfig.Get<IAbstractModuleConfig>(moduleName);
    }

public:

    TConfig()
        : ModulesConfig(*this, "ModulesConfig")
    {

    }

    TConfig(TProcessId parentPid, TDuration checkInterval = TDuration::Seconds(10))
        : ModulesConfig(*this, "ModulesConfig")
    {
        TStringStream ss;
        ss << "<ModulesConfig>" << Endl;
        ss << "<ParentExistenceChecker>" << Endl;
        ss << "Enabled: " << true << Endl;
        ss << "ParentPid: " << parentPid << Endl;
        ss << "CheckInterval:" << checkInterval.Seconds() << Endl;
        ss << "</ParentExistenceChecker>" << Endl;
        ss << "</ModulesConfig>" << Endl;
        ss << "<Server>" << Endl;
        ss << "State: OK" << Endl;
        ss << "</Server>" << Endl;
        InitFromString(ss.Str());
    }

    TConfig(const TServerConfigConstructorParams& params)
        : DaemonConfig(*params.GetDaemonConfig())
        , ModulesConfig(*this, "ModulesConfig")
    {
        if (!!params.GetText())
            InitFromString(params.GetText());
    }

    TString ToString() const {
        TStringStream ss;
        ss << DaemonConfig.ToString("DaemonConfig") << Endl;
        ss << "<Server>" << Endl;
        ss << "State: " << State << Endl;
        ss << "</Server>" << Endl;
        ModulesConfig.ToString(&ss);
        return ss.Str();
    }

    virtual TSet<TString> GetModulesSet() const override {
        return UsingModules;
    }

    TDaemonConfig& MutableDaemonConfig() {
        return DaemonConfig;
    }

    const TString& GetState() const {
        return State;
    }

    void SetState(const TString& value) {
        State = value;
    }

    virtual const TDaemonConfig& GetDaemonConfig() const override {
        return DaemonConfig;
    }
};
