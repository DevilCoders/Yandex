#include "patcher.h"
#include "its_config.h"

#include <kernel/daemon/common/environment.h>

#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/yaml/scheme/traits.h>

#include <util/digest/murmur.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/system/execpath.h>
#include <util/system/hostname.h>
#include <util/system/info.h>
#include <util/string/subst.h>
#include <util/system/env.h>
#include <util/charset/utf8.h>

TString TConfigPatcher::DetectDatacenterCode() {
    {
        const TString externalDC = ToUpperUTF8(GetEnv("DEPLOY_NODE_DC"));
        if (!!externalDC) {
            return externalDC;
        }
    }
    const TString fqdnHostName = GetFQDNHostName();
    const TString hostName = GetHostName();
    if (fqdnHostName.find(".man.") != TString::npos) {
        return "MAN";
    } else if (fqdnHostName.find(".sas.") != TString::npos) {
        return "SAS";
    } else if (fqdnHostName.find(".vla.") != TString::npos) {
        return "VLA";
    } else if (hostName.find("man") != TString::npos) {
        return "MAN";
    } else if (hostName.find("sas") != TString::npos) {
        return "SAS";
    } else if (hostName.find("vla") != TString::npos) {
        return "VLA";
    }
    return "MSK";
}

TConfigPatcher::TConfigPatcher(const TString& patchPrefix)
    : PatchPrefix(patchPrefix)
{
    const TString& host = HostName();

    // add several system variables
    SetVariable("NCPU", ToString(NSystemInfo::CachedNumberOfCpus()));
    SetVariable("PAGESIZE", ToString(NSystemInfo::GetPageSize()));
    SetVariable("_BIN_DIRECTORY", TFsPath(GetExecPath()).Dirname());
    SetVariable("HOSTNAME", host);
    SetVariable("HOSTNAME_HASH", ToString(MurmurHash<ui32>(host.data(), host.size())));
    SetVariable("DATACENTER_CODE", DetectDatacenterCode());

    // set environment variables
    for (auto&& variable : NUtil::GetEnvironmentVariables()) {
        SetVariable(variable.first, variable.second);
    }
}

TString TConfigPatcher::ReadAndProcess(const TString& filename) {
    THashMap<TString, TString> itsPatches;
    if (ItsConfigPaths) {
        HandleItsConfig(ItsConfigPaths, TFsPath(filename).Dirname(), itsPatches);
    }
    {
        TUnstrictConfig unsConf;
        if (unsConf.Parse(filename)) {
            auto children = unsConf.GetRootSection()->GetAllChildren();
            auto it = children.find("InternalVariables");
            if (it != children.end()) {
                for (auto&& i : it->second->GetDirectives()) {
                    Preprocessor.SetVariable(i.first, i.second);
                }
            }
        }
    }
    const TString preprocessed = Preprocessor.ReadAndProcess(filename);
    TUnstrictConfig unsConf;
    if (!unsConf.ParseMemory(preprocessed.data())) {
        TString err;
        unsConf.PrintErrors(err);
        ythrow yexception() << "Cannot parse config: " << err;
    }

    for (THashMap<TString, TString>::const_iterator i = Patches.begin(); i != Patches.end(); ++i) {
        TString patchLine(i->second);
        Preprocessor.ProcessLine(patchLine);
        unsConf.PatchEntry(i->first, patchLine, PatchPrefix);
    }
    for (const auto& patch : itsPatches) {
        TString patchLine(patch.second);
        Preprocessor.ProcessLine(patchLine);
        unsConf.PatchEntry(patch.first, patchLine, PatchPrefix);
    }
    return unsConf.ToString();
}

void TConfigPatcher::ReadSecdist(const TString& filename, const TString& service, const TString& dbname, const THashMap<TString, TString>& varsPaths) {
    TUnbufferedFileInput fi(filename);
    TString content = fi.ReadAll();

    NJson::TJsonValue json;
    if (!NJson::ReadJsonFastTree(content, &json)) {
        Cdbg << "Incorrect secdist format";
        return;
    }

    TSet<TString> serviceNames;
    serviceNames.emplace(service);
    serviceNames.emplace(SubstGlobalCopy(service, "-", "_"));
    serviceNames.emplace(SubstGlobalCopy(service, "_", "-"));

    {
        const auto& databases = json["postgresql_settings"]["databases"];
        for (auto&& serviceJson : databases.GetMap()) {
            if (serviceNames.contains(serviceJson.first) || !dbname.Empty() && dbname == serviceJson.first) {
                for (auto&& config : serviceJson.second.GetArray()) {
                    if (config["hosts"].GetArray().size() > 0) {
                        TString connString = config["hosts"].GetArray().front().GetString();
                        SetVariable("SecDistPSQLConn", connString, true);
                        break;
                    }
                }
            }
        }
    }
    {
        const auto& settings = json["settings_override"];
        for (auto&& serviceJson : settings["TVM_SERVICES"].GetMap()) {
            if (serviceNames.contains(serviceJson.first)) {
                const auto& config = serviceJson.second;
                if (config.Has("id")) {
                    SetVariable("SecDistTVMId", config["id"].GetStringRobust(), true);
                }
                if (config.Has("secret")) {
                    SetVariable("SecDistTVMSecret", config["secret"].GetStringRobust(), true);
                }
            }
        }
    }
    if (!varsPaths.empty()) {
        for (const auto& [var, path] : varsPaths) {
            if (const auto* value = json.GetValueByPath(path, '/')) {
                if (var.EndsWith('*')) {
                    const auto varPrefix = var.substr(0, var.length() - 1);
                    for (const auto& [k, v]: value->GetMap()) {
                        SetVariable(varPrefix + k, v.GetStringRobust(), true);
                    }
                } else {
                    SetVariable(var, value->GetStringRobust(), true);
                }
            }
        }
    }
}

void TConfigPatcher::ReadEnvironment(const TString& filename, bool push) {
    if (TFsPath(filename).GetExtension() == "yaml") {
        ReadEnvironmentFromYaml(filename);
    } else {
        ReadEnvironmentPlain(filename);
    }
    if (push) {
        EnvironmentFiles.push_back(filename);
    }
}

void TConfigPatcher::ReadEnvironmentPlain(const TString& filename) {
    TFileInput fi(filename);
    TString line;
    while (fi.ReadLine(line)) {
        TStringBuf k, v;
        if (TStringBuf(line).TrySplit('=', k, v)) {
            SetVariable(k, v, true);
        }
    }
}

void TConfigPatcher::ReadEnvironmentFromYaml(const TString& filename) {
    TFileInput fi(filename);
    TYamlDocument doc(std::move(fi));
    auto v = doc.Root();
    if (TYamlTraits::IsDict(v)) {
        for (auto it = TYamlTraits::DictBegin(v); it != TYamlTraits::DictEnd(v); ++it) {
            SetVariable(TYamlTraits::DictIteratorKey(v, it), TYamlTraits::DictIteratorValue(v, it).Scalar(), true);
        }
    }
}

void TConfigPatcher::ReReadEnvironment() {
    for (const auto& i: EnvironmentFiles) {
        ReadEnvironment(i, false);
    }
}

void TConfigPatcher::SetVariable(const TStringBuf name, const TStringBuf value, const bool setEnv) {
    Cdbg << "Variable: " << name << ": " << value << Endl;
    const TString n(name);
    const TString v(value);
    Preprocessor.SetVariable(n, v);
    if (setEnv) {
        SetEnv(n, v);
    }
}

void TConfigPatcher::AddPatch(const TString& path, const TString& value) {
    Cdbg << "Patch: " << path << ": " << value << Endl;
    Patches[path] = value;
}
