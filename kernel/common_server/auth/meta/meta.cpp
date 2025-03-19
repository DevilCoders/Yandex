#include "meta.h"

#include <util/string/split.h>

IAuthInfo::TPtr TMetaAuthModule::DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const {
    IAuthInfo::TPtr lastUnavailableInfo = nullptr;
    for (auto&& [name, module] : Submodules) {
        if (!module) {
            TFLEventLog::Info("Null pointer to authorization module.")("module_name", name);
            continue;
        }
        auto info = module->RestoreAuthInfo(requestContext);
        if (!info) {
            TFLEventLog::Info("Authorization module cannot restore AuthInfo.")("module_name", name);
            continue;
        }
        if (!info->IsAvailable()) {
            TFLEventLog::Info("Authorization info is not available.")("module_name", name)
                             ("module_message", info->GetMessage());
            lastUnavailableInfo = info;
            continue;
        }
        return info;
    }
    return lastUnavailableInfo;
}

THolder<IAuthModule> TMetaAuthConfig::DoConstructAuthModule(const IBaseServer* server) const {
    TMetaAuthModule::TSubmodules submodules;
    for (auto&& name : Submodules) {
        if (name == GetModuleName()) {
            ERROR_LOG << "self-inclusion in MetaAuthConfig: " << GetModuleName() << Endl;
            continue;
        }

        const IAuthModuleConfig* config = server ? server->GetAuthModuleInfo(name) : nullptr;
        if (!config) {
            ERROR_LOG << "null AuthModuleConfig: " << name << Endl;
            continue;
        }
        submodules.emplace_back(name, config->ConstructAuthModule(server));
    }
    return MakeHolder<TMetaAuthModule>(std::move(submodules));
}

void TMetaAuthConfig::DoInit(const TYandexConfig::Section* section) {
    CHECK_WITH_LOG(section);
    const auto& directives = section->GetDirectives();
    Submodules = StringSplitter(directives.Value("Submodules", JoinStrings(Submodules, ",")))
        .SplitBySet(", ")
        .SkipEmpty();
}

void TMetaAuthConfig::DoToString(IOutputStream& os) const {
    os << "Submodules: " << JoinStrings(Submodules, ",") << Endl;
}

TMetaAuthConfig::TFactory::TRegistrator<TMetaAuthConfig> TMetaAuthConfig::Registrator("meta");
