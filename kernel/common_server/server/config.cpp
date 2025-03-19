#include "config.h"

#include <library/cpp/mediator/global_notifications/system_status.h>
#include <library/cpp/logger/thread_creator.h>
#include <library/cpp/logger/filter_creator.h>
#include <kernel/common_server/auth/common/tvm_config.h>
#include <kernel/common_server/util/logging/backend_creator_helpers.h>
#include <kernel/common_server/util/enum_cast.h>

#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/string/join.h>

namespace {
    void PrintConfigs(const TYandexConfig::Section& mainSection, IOutputStream& os, TSet<TString>& readyObjects) {
        for (const auto& iter : mainSection.GetDirectives()) {
            if (iter.second != nullptr && *iter.second && !IsSpace(iter.second)) {
                if (readyObjects.emplace("d:" + iter.first).second) {
                    os << iter.first << " " << iter.second << "\n";
                }
            }
        }
        auto children = mainSection.GetAllChildren();
        for (auto&& i : children) {
            if (readyObjects.emplace("s:" + i.first).second) {
                TYandexConfig::PrintSectionConfig(i.second, os, false);
            }
        }
    }

    void MergeAndPrintConfigs(const TYandexConfig::Section* mainSection, IOutputStream& os, const TYandexConfig::Section* secondary, const TString& sectionName) {
        if (!!sectionName) {
            os << "<" << sectionName << ">" << Endl;
        }
        TSet<TString> readyObjects;
        if (mainSection) {
            PrintConfigs(*mainSection, os, readyObjects);
        }
        if (secondary) {
            PrintConfigs(*secondary, os, readyObjects);
        }
        if (!!sectionName) {
            os << "</" << sectionName << ">" << Endl;
        }
    }
}

void TBaseServerConfig::Init(const TYandexConfig::Section* section) {
    try {
        const TYandexConfig::Directives& directives = section->GetDirectives();
        const TYandexConfig::TSectionsMap serverSections = section->GetAllChildren();
        NoPermissionsModeFlag = directives.Value("NoPermissionsMode", NoPermissionsModeFlag);
        {
            auto it = serverSections.find("CertsManagerConfig");
            if (it != serverSections.end()) {
                CertsManagerConfig.Init(it->second);
            }
        }
        {
            auto it = serverSections.find("PropositionsManagerConfig");
            if (it != serverSections.end()) {
                PropositionsManagerConfig.Init(it->second);
            }
        }
        {
            auto it = serverSections.find("ObfuscatorManagerConfig");
            if (it != serverSections.end()) {
                ObfuscatorManagerConfig.Init(it->second);
            }
        }
        {
            auto it = serverSections.find("ResourcesManagerConfig");
            if (it != serverSections.end()) {
                ResourcesManagerConfig.Init(it->second);
            }

        }
        {
            auto it = serverSections.find("GeobaseConfig");
            if (it != serverSections.end()) {
                GeobaseConfig.Init(it->second);
            }
        }

        EventLog.Init(section);

        {
            auto it = serverSections.find("SnapshotsController");
            if (it != serverSections.end()) {
                SnapshotsControllerConfig.Init(it->second);
            }
        }
        {
            auto it = serverSections.find("PersonalDataStorage");
            if (it != serverSections.end()) {
                AssertCorrectConfig(PersonalDataStorageConfig.Init(it->second), "Can't parse PersonalDataStorage");
            }
        }

        {
            auto itADConfig = serverSections.find("GlobalAsyncDelivery");
            if (itADConfig != serverSections.end()) {
                ADGlobalConfig = MakeHolder<TAsyncDeliveryResources::TConfig>();
                ADGlobalConfig->Init(itADConfig->second);
                if (GetADGlobalConfig()) {
                    TAsyncDeliveryResources::InitGlobalResources(*GetADGlobalConfig());
                }
            }
        }

        {
            auto it = serverSections.find("Signatures");
            if (it != serverSections.end()) {
                ElSignaturesConfig.Init(it->second);
            }
        }

        {
            auto it = serverSections.find("RequestPolicy");
            if (it != serverSections.end()) {
                const TYandexConfig::TSectionsMap requestsPolicySection = it->second->GetAllChildren();
                for (auto&& i : requestsPolicySection) {
                    NSimpleMeta::TConfig config;
                    config.InitFromSection(i.second);
                    AssertCorrectConfig(!RequestPolicy.contains(i.first), "Policy duplication: %s", i.first.data());
                    RequestPolicy.emplace(i.first, std::move(config));
                }
            }
        }

        {
            auto it = serverSections.find("AuthUsersManager");
            if (it == serverSections.end()) {
                it = serverSections.find("UsersManager");
                AssertCorrectConfig(it != serverSections.end(), "Incorrect AuthUsersManager config");
            }
            AuthUsersManagerConfig.Init(it->second);
        }
        {
            auto it = serverSections.find("Notifications");
            if (it != serverSections.end()) {
                const TYandexConfig::TSectionsMap sections = it->second->GetAllChildren();
                for (auto&& i : sections) {
                    TString typeName;
                    if (!i.second->GetDirectives().GetValue("NotificationType", typeName)) {
                        typeName = i.first;
                    }

                    IFrontendNotifierConfig::TPtr generator = IFrontendNotifierConfig::TFactory::Construct(typeName);
                    AssertCorrectConfig(!!generator, "Incorrect typename for notification: " + typeName);

                    generator->SetTypeName(typeName);
                    generator->SetName(i.first);
                    generator->Init(i.second);

                    Notifications.emplace(i.first, generator);
                }
            }
        }

        {
            auto itHandlers = serverSections.find("Localization");
            if (itHandlers != serverSections.end()) {
                Localization = MakeHolder<NCS::NLocalization::TConfig>();
                Localization->Init(itHandlers->second);
            }
        }

        {
            auto itHandlers = serverSections.find("Settings");
            if (itHandlers != serverSections.end()) {
                Settings.Init(itHandlers->second);
            }
        }

        {
            auto itHandlers = serverSections.find("Databases");
            if (itHandlers != serverSections.end()) {
                const TYandexConfig::TSectionsMap dBases = itHandlers->second->GetAllChildren();
                {
                    for (auto&& i : dBases) {
                        NRTProc::TStorageOptions options;
                        AssertCorrectConfig(options.Init(i.second), "Incorrect section '%s' in Databases", i.first.data());
                        DatabaseConfigs.emplace(i.first, options);
                    }
                }
            }
        }

        {
            auto itHandlers = serverSections.find("ExternalDatabases");
            if (itHandlers != serverSections.end()) {
                const TYandexConfig::TSectionsMap dBases = itHandlers->second->GetAllChildren();
                for (auto&& i : dBases) {
                    NStorage::TDatabaseConfig dbConfig;
                    dbConfig.Init(i.second);
                    Databases.emplace(i.first, dbConfig);
                    NCS::NStorage::TDBMigrationsConfig migrationsCofig;
                    const auto children = i.second->GetAllChildren();
                    if (const auto* mig = MapFindPtr(children, "Migrations")) {
                        migrationsCofig.Init(*mig, i.first);
                    }
                    if (!migrationsCofig.GetMigrationSources().empty()) {
                        DBMigrationManagerConfigs.emplace(i.first, std::move(migrationsCofig));
                    }
                }
            }
        }

        TString httpUserAgent;
        directives.GetValue("UserAgent", httpUserAgent);
        {
            auto it = serverSections.find("ExternalQueues");
            if (it != serverSections.end()) {
                const TYandexConfig::TSectionsMap apiList = it->second->GetAllChildren();
                for (auto&& i : apiList) {
                    NCS::TPQClientConfigContainer c;
                    c.Init(i.second);
                    AssertCorrectConfig(!!c, "cannot initialize queue config for %s", i.first.data());
                    AssertCorrectConfig(ExternalQueueConfigs.emplace(c->GetClientId(), c).second, "queue config ids duplication");
                }
            }
        }

        {
            auto it = serverSections.find("SecretManagers");
            if (it != serverSections.end()) {
                const TYandexConfig::TSectionsMap secretManagers = it->second->GetAllChildren();
                for (auto&& i : secretManagers) {
                    NCS::NSecret::TSecretsManagerConfig config;
                    config.Init(i.second);
                    SecretManagerConfigs.emplace(i.first, std::move(config));
                }
            }
        }

        {
            auto it = serverSections.find("AbstractExternalAPI");
            if (it != serverSections.end()) {
                const TYandexConfig::TSectionsMap apiList = it->second->GetAllChildren();
                for (auto&& i : apiList) {
                    NExternalAPI::TSenderConfig config;
                    config.Init(i.second, &RequestPolicy);
                    config.SetHttpUserAgent(httpUserAgent);
                    AbstractExternalAPI.emplace(i.first, std::move(config));
                }
            }
        }

        {
            auto it = serverSections.find("RolesManager");
            if (it != serverSections.end()) {
                RolesManagerConfig.Init(it->second);
            } else {
                RolesManagerConfig.Init(nullptr);
            }
        }

        {
            auto it = serverSections.find("PermissionsManager");
            if (it != serverSections.end()) {
                PermissionsManagerConfig.Init(it->second);
            } else {
                PermissionsManagerConfig.Init(nullptr);
            }
        }

        {
            auto it = serverSections.find("Monitoring");
            AssertCorrectConfig(it != serverSections.end(), "No 'Monitoring' section in config");
            MonitoringConfig.Init(it->second);
        }

        {
            auto it = serverSections.find("Emulations");
            if (it != serverSections.end()) {
                EmulationsManagerConfig.Init(it->second);
            }
        }

        {
            auto it = serverSections.find("TagDescriptions");
            if (it != serverSections.end()) {
                TagDescriptionsConfig.Init(it->second);
            }
        }

        {
            auto itProcessors = serverSections.find("Processors");
            AssertCorrectConfig(itProcessors != serverSections.end(), "No 'Processors' section in config");
            const TYandexConfig::TSectionsMap children = itProcessors->second->GetAllChildren();

            TMap<TString, const TYandexConfig::Section*> defaultSections;
            for (auto&& i : children) {
                if (i.first.StartsWith("default_config:")) {
                    defaultSections[Strip(i.first.substr(TString("default_config:").size()))] = i.second;
                }
            }

            for (auto&& i : children) {
                if (i.first.StartsWith("default_config:")) {
                    continue;
                }
                const TYandexConfig::Section* defaultSection = nullptr;
                TString sectionName = Strip(i.first);
                size_t separatorPos = i.first.find(":");
                if (separatorPos != TString::npos) {
                    sectionName = Strip(i.first.substr(0, separatorPos));
                    const TString defSectionName = Strip(i.first.substr(separatorPos + 1));
                    auto defaultSectionIt = defaultSections.find(defSectionName);
                    AssertCorrectConfig(defaultSectionIt != defaultSections.end(), "incorrect default section %s for %s", defSectionName.data(), sectionName.data());
                    defaultSection = defaultSectionIt->second;
                } else {
                    auto it = defaultSections.find("for_all");
                    if (it != defaultSections.end()) {
                        defaultSection = it->second;
                    }
                }
                TStringStream ss;
                MergeAndPrintConfigs(i.second, ss, defaultSection, sectionName);
                TAnyYandexConfig yConfig;
                CHECK_WITH_LOG(yConfig.ParseMemory(ss.Str()));
                auto children = yConfig.GetRootSection()->GetAllChildren();
                auto it = children.find(sectionName);
                CHECK_WITH_LOG(it != children.end());
                auto sectionRoot = it->second;

                const TString typeName = sectionRoot->GetDirectives().Value("ProcessorType", sectionName);
                THolder<IRequestProcessorConfig> generator(IRequestProcessorConfig::TFactory::Construct(typeName, sectionName));
                AssertCorrectConfig(!!generator, "Incorrect processor type: '%s' in section '%s'", typeName.data(), sectionName.data());
                AssertCorrectConfig(generator->Init(sectionRoot), "Cannot initialize processor: '%s' in section '%s'", typeName.data(), sectionName.data());
                ProcessorsInfo.emplace(sectionName, std::move(generator));
            }
        }

        {
            auto itProcessors = serverSections.find("AuthModules");
            if (itProcessors != serverSections.end()) {
                const TYandexConfig::TSectionsMap children = itProcessors->second->GetAllChildren();
                for (auto&& i : children) {
                    TString authType;
                    AssertCorrectConfig(i.second->GetDirectives().GetValue("type", authType), "No auth type for %s", i.first.data());
                    THolder<IAuthModuleConfig> generator(IAuthModuleConfig::TFactory::Construct(authType, i.first));
                    AssertCorrectConfig(!!generator, "Incorrect auth module type: '%s'", i.first.data());
                    generator->Init(i.second);
                    AuthModuleConfigs.emplace(i.first, std::move(generator));
                }
            }
        }

        {
            auto it = serverSections.find("NotifiersManager");
            if (it != serverSections.end()) {
                NotifiersManagerConfig = MakeHolder<TNotifiersManagerConfig>();
                NotifiersManagerConfig->Init(it->second);
            }
        }

        {
            auto it = serverSections.find("SecretsManager");
            if (it != serverSections.end()) {
                SecretsManagerConfig = MakeHolder<TSecretsManagerConfig>();
                SecretsManagerConfig->Init(it->second);
            }
        }

        {
            auto it = serverSections.find("RTBackgroundManager");
            if (it != serverSections.end()) {
                RTBackgroundManagerConfig = MakeHolder<TRTBackgroundManagerConfig>();
                RTBackgroundManagerConfig->Init(it->second);
            }
        }

        {
            auto itHttpServer = serverSections.find("HttpServer");
            AssertCorrectConfig(itHttpServer != serverSections.end(), "No 'HttpServer' section in config");
            HttpServerOptions.Init(itHttpServer->second->GetDirectives());
        }

        {
            auto itRequestProcessing = serverSections.find("RequestProcessing");
            AssertCorrectConfig(itRequestProcessing != serverSections.end(), "No 'RequestProcessing' section in config");
            RequestProcessingConfig.Init(itRequestProcessing->second);
        }

        {
            auto itHttpStatuses = serverSections.find("HttpStatuses");
            if (itHttpStatuses != serverSections.end()) {
                HttpStatusManagerConfig.Init(itHttpStatuses->second);
            }
        }

        {
            auto itHandlers = serverSections.find("RequestHandlers");
            AssertCorrectConfig(itHandlers != serverSections.end(), "No 'RequestHandlers' section in config");

            const TYandexConfig::TSectionsMap handlers = itHandlers->second->GetAllChildren();
            {
                bool hasDefaultHandler = false;
                for (auto&& i : handlers) {
                    if (i.first == "default") {
                        hasDefaultHandler = true;
                    }
                    RequestHandlers[i.first].Init(i.second);
                }
                AssertCorrectConfig(hasDefaultHandler, "No 'default' section in 'RequestHandlers'");
            }
        }

        {
            auto it = serverSections.find("LocksManager");
            if (it != serverSections.end()) {
                LocksManagerConfigContainer.Init(it->second);
            } else {
                LocksManagerConfigContainer.BuildFake();
            }
        }

        {
            auto itModulesConfig = serverSections.find("ModulesConfig");
            if (itModulesConfig != serverSections.end()) {
                ModulesConfig.Init(*this, serverSections);
                for (auto&& module : itModulesConfig->second->GetAllChildren()) {
                    UsingModules.insert(module.first);
                }
            }

            for (const auto& itAuthConfig : AuthModuleConfigs) {
                const TSet<TString>& modules = itAuthConfig.second->GetRequiredModules();
                for (auto& module : modules) {
                    AssertCorrectConfig(UsingModules.find(module) != UsingModules.end(), "Daemon module '%s' not found, required for auth module '%s'", module.data(), itAuthConfig.first.data());
                }
            }
        }

        {
            for (auto&& [name, section] : MakeIteratorRange(serverSections.equal_range("Tvm"))) {
                TTvmConfig config;
                config.Init(section);
                AssertCorrectConfig(config.GetSelfClientId() != 0, "incorrect Tvm section: SelfClientId is zero or missing");
                AssertCorrectConfig(TvmConfigs.emplace(config.GetName(), std::move(config)).second, "incorrect Tvm section: duplicate Name %s", config.GetName().c_str());
            }
        }

        {
            auto it = serverSections.find("Ciphers");
            if (it != serverSections.end()) {
                const TYandexConfig::TSectionsMap cipherSection = it->second->GetAllChildren();
                for (auto&& i : cipherSection) {
                    TString cipherType;
                    AssertCorrectConfig(i.second->GetDirectives().GetValue("type", cipherType), "No cipher type for %s", i.first.data());
                    THolder<NCS::ICipherConfig> generator(NCS::ICipherConfig::TFactory::Construct(cipherType, i.first));
                    AssertCorrectConfig(!!generator, "Incorrect cipher type: '%s'", i.first.data());
                    generator->Init(i.second);
                    Ciphers.emplace(i.first, std::move(generator));
                }
            }
        }

        {
            auto it = serverSections.find("GeoAreas");
            if (it != serverSections.end()) {
                GeoAreasConfig.Init(it->second);
            }
        }
    } catch (...) {
        S_FAIL_LOG << CurrentExceptionMessage() << Endl;
    }
}

TBaseServerConfig::TBaseServerConfig(const TServerConfigConstructorParams& params)
    : DaemonConfig(*params.GetDaemonConfig())
    , ModulesConfig(*this, "ModulesConfig")
{
}

void TBaseServerConfig::OnAfterCreate(const TServerConfigConstructorParams& params) {
    TAnyYandexConfig config;
    CHECK_WITH_LOG(config.ParseMemory(params.GetText().data()));
    const TYandexConfig::Section* root = config.GetRootSection();
    const TYandexConfig::TSectionsMap rootSections = root->GetAllChildren();
    auto itServer = rootSections.find("Server");
    AssertCorrectConfig(itServer != rootSections.end(), "No 'Server' section in config");
    Init(itServer->second);
}

const TMap<TString, NStorage::TDatabaseConfig>& TBaseServerConfig::GetExternalDatabases() const {
    return Databases;
}

void TBaseServerConfig::ToString(IOutputStream& os) const {
    os << DaemonConfig.ToString("DaemonConfig") << Endl;
    os << "<Server>" << Endl;
    os << "NoPermissionsMode: " << NoPermissionsModeFlag << Endl;
    os << HttpServerOptions.ToString("HttpServer");

    os << "<RequestProcessing>" << Endl;
    RequestProcessingConfig.ToString(os);
    os << "</RequestProcessing>" << Endl;
    os << "<Signatures>" << Endl;
    ElSignaturesConfig.ToString(os);
    os << "</Signatures>" << Endl;

    os << "<SnapshotsController>" << Endl;
    SnapshotsControllerConfig.ToString(os);
    os << "</SnapshotsController>" << Endl;

    os << "<RequestPolicy>" << Endl;
    for (auto&& i : RequestHandlers) {
        os << "<" << i.first << ">" << Endl;
        i.second.ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</RequestPolicy>" << Endl;

    os << "<Notifications>" << Endl;
    for (auto&& i : Notifications) {
        os << "<" << i.first << ">" << Endl;
        i.second->ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</Notifications>" << Endl;

    os << "<RequestHandlers>" << Endl;
    for (auto&& i : RequestHandlers) {
        os << "<" << i.first << ">" << Endl;
        i.second.ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</RequestHandlers>" << Endl;

    os << "<AuthUsersManager>" << Endl;
    AuthUsersManagerConfig.ToString(os);
    os << "</AuthUsersManager>" << Endl;

    if (!!Localization) {
        os << "<Localization>" << Endl;
        Localization->ToString(os);
        os << "</Localization>" << Endl;
    }

    os << "<TagDescriptions>" << Endl;
    TagDescriptionsConfig.ToString(os);
    os << "</TagDescriptions>" << Endl;

    if (!!Settings) {
        os << "<Settings>" << Endl;
        Settings.ToString(os);
        os << "</Settings>" << Endl;
    }

    os << "<Databases>" << Endl;
    for (auto&& i : DatabaseConfigs) {
        os << "<" << i.first << ">" << Endl;
        i.second.ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</Databases>" << Endl;

    os << "<ExternalDatabases>" << Endl;
    for (const auto& [name, db] : Databases) {
        os << "<" << name << ">" << Endl;
        db.ToString(os);
        if (const auto* mig = MapFindPtr(DBMigrationManagerConfigs, name)) {
            os << "<Migrations>" << Endl;
            mig->ToString(os);
            os << "</Migrations>" << Endl;
        }
        os << "</" << name << ">" << Endl;
    }
    os << "</ExternalDatabases>" << Endl;

    os << "<RolesManager>" << Endl;
    RolesManagerConfig.ToString(os);
    os << "</RolesManager>" << Endl;

    os << "<PermissionsManager>" << Endl;
    PermissionsManagerConfig.ToString(os);
    os << "</PermissionsManager>" << Endl;

    os << "<Monitoring>" << Endl;
    MonitoringConfig.ToString(os);
    os << "</Monitoring>" << Endl;

    os << "<Processors>" << Endl;
    for (auto&& i : ProcessorsInfo) {
        os << "<" << i.first << ">" << Endl;
        i.second->ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</Processors>" << Endl;

    os << "<AuthModules>" << Endl;
    for (auto&& i : AuthModuleConfigs) {
        os << "<" << i.first << ">" << Endl;
        i.second->ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</AuthModules>" << Endl;

    os << "<LocksManager>" << Endl;
    LocksManagerConfigContainer.ToString(os);
    os << "</LocksManager>" << Endl;

    if (RTBackgroundManagerConfig) {
        os << "<RTBackgroundProcesses>" << Endl;
        RTBackgroundManagerConfig->ToString(os);
        os << "</RTBackgroundProcesses>" << Endl;
    }

    if (NotifiersManagerConfig) {
        os << "<NotifiersManager>" << Endl;
        NotifiersManagerConfig->ToString(os);
        os << "</NotifiersManager>" << Endl;
    }


    if (SecretsManagerConfig) {
        os << "<SecretsManager>" << Endl;
        SecretsManagerConfig->ToString(os);
        os << "</SecretsManager>" << Endl;
    }

    os << "<Ciphers>" << Endl;
    for (auto&& cipherIt : Ciphers) {
        if (!cipherIt.second) {
            continue;
        }
        os << "<" << cipherIt.first << ">" << Endl;
        cipherIt.second->ToString(os);
        os << "</" << cipherIt.first << ">" << Endl;
    }
    os << "</Ciphers>" << Endl;

    os << "<ExternalQueues>" << Endl;
    for (auto&& i : ExternalQueueConfigs) {
        os << "<" << i.first << ">" << Endl;
        i.second.ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</ExternalQueues>" << Endl;

    os << "<SecretManagers>" << Endl;
    for (auto&& i : SecretManagerConfigs) {
        os << "<" << i.first << ">" << Endl;
        i.second.ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</SecretManagers>" << Endl;

    os << "<AbstractExternalAPI>" << Endl;
    for (auto&& i : AbstractExternalAPI) {
        os << "<" << i.first << ">" << Endl;
        i.second.ToString(os);
        os << "</" << i.first << ">" << Endl;
    }
    os << "</AbstractExternalAPI>" << Endl;

    if (!!GeoAreasConfig) {
        os << "<GeoAreas>" << Endl;
        GeoAreasConfig.ToString(os);
        os << "</GeoAreas>" << Endl;
    }

    EventLog.ToString(os);

    if (!!ResourcesManagerConfig.GetDBName()) {
        os << "<ResourcesManagerConfig>" << Endl;
        ResourcesManagerConfig.ToString(os);
        os << "</ResourcesManagerConfig>" << Endl;
    }
    if (!!PropositionsManagerConfig.GetDBName()) {
        os << "<PropositionsManagerConfig>" << Endl;
        PropositionsManagerConfig.ToString(os);
        os << "</PropositionsManagerConfig>" << Endl;
    }
    if (!!ObfuscatorManagerConfig.GetDBName()) {
        os << "<ObfuscatorManagerConfig>" << Endl;
        ObfuscatorManagerConfig.ToString(os);
        os << "</ObfuscatorManagerConfig>" << Endl;
    }
    
    os << "<CertsManagerConfig>" << Endl;
    CertsManagerConfig.ToString(os);
    os << "</CertsManagerConfig>" << Endl;

    os << "<Geobase>" << Endl;
    GeobaseConfig.ToString(os);
    os << "</Geobase>" << Endl;

    DoToString(os);
    os << "</Server>" << Endl;
}

THolder<TLogBackend> TEventLogConfig::CreateLogBackend() const {
    return BackendCreator->CreateLogBackend();
}

void TEventLogConfig::Init(const TYandexConfig::Section* section) {
    BackendCreator = NCommonServer::NUtil::CreateLogBackendCreator(*section, "Logging", "EventLog");
    AssertCorrectConfig(!!BackendCreator, "Eventlog backend not created");
    const auto children = section->GetAllChildren();
    if (const auto* logging = MapFindPtr(children, "Logging")) {
        const auto& dir = (*logging)->GetDirectives();
        BackgroundUsageFlag = dir.Value("BackgroundUsage", BackgroundUsageFlag);
        SkipOnQueueLimitExceeded = dir.Value("SkipOnQueueLimitExceeded", SkipOnQueueLimitExceeded);
        QueueSizeLimit = dir.Value("QueueSizeLimit", QueueSizeLimit);

        const TString priority = dir.Value<TString>("Priority", ::ToString(Priority));
        AssertCorrectConfig(NCS::TEnumWorker<ELogPriority>::TryParseFromString(priority, Priority), "incorrect log Priority");
        dir.GetValue("Format", Format);
    }
    BackendCreator = MakeHolder<TOwningThreadedLogBackendCreator>(std::move(BackendCreator));
}

void TEventLogConfig::ToString(IOutputStream& os) const {
    os << "<Logging>" << Endl;
    NCommonServer::NUtil::LogBackendCreatorToString(BackendCreator, "", os);
    os << "Priority: " << Priority << Endl;
    os << "BackgroundUsage: " << BackgroundUsageFlag << Endl;
    os << "Format: " << Format << Endl;
    os << "</Logging>" << Endl;
}


void TRequestProcessingConfig::Init(const TYandexConfig::Section* section) {
    const auto& dir = section->GetDirectives();
    dir.GetValue("DefaultType", DefaultType);
    StringSplitter(dir.Value("HeadersToForward", TString())).SplitBySet(", ").SkipEmpty().Collect(&HeadersToForward);
}

void TRequestProcessingConfig::ToString(IOutputStream& os) const {
    os << "DefaultType: " << DefaultType << Endl;
    os << "HeadersToForward: " << JoinSeq(", ", HeadersToForward) << Endl;
}
