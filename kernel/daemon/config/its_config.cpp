#include "its_config.h"

#include <kernel/daemon/common/environment.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/logger/global/global.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/split.h>


namespace {
    void DumpJsonItem(IOutputStream &stream, const TString &name, const NJson::TJsonValue &json) {
        if (json.IsMap()) {
            stream << "<" << name << ">" << Endl;
            for (const auto &i: json.GetMap()) {
                DumpJsonItem(stream, i.first, i.second);
            }
            stream << "</" << name << ">" << Endl;
        } else if (json.IsArray()) {
            for (const auto &i: json.GetArray()) {
                DumpJsonItem(stream, name, i);
            }
        } else if (json.IsBoolean()) {
            stream << name << ": " << (json.GetBoolean() ? "true" : "false") << Endl;
        } else if (json.IsInteger()) {
            stream << name << ": " << json.GetInteger() << Endl;
        } else if (json.IsDouble()) {
            stream << name << ": " << json.GetDouble() << Endl;
        } else if (json.IsString()) {
            stream << name << ": " << json.GetString() << Endl;
        }
    }

    bool CheckFileName(const TString& name) {
        if (!name) {
            ERROR_LOG << "Empty file name is illegal" << Endl;
            return false;
        }
        if (name.Contains("..")) {
            ERROR_LOG << "Invalid file name for yconf include: " << name << Endl;
            return false;
        }
        return true;
    }
}

void HandleItsPatches(
        const NJson::TJsonValue& json, 
        THashMap<TString, TString>& patches, 
        const TString& section
) noexcept {
    if (!json.Has(section))
        return;
    const auto& patchesJson = json[section];
    for (const auto& p : patchesJson.GetMap()) {
        const auto& name = p.first;
        const auto& value = p.second.GetString();
        patches[name] = value;
    }
}

void HandleItsConfig(
        const TVector<TString>& itsConfigPaths,
        const TString& dstFolder,
        THashMap<TString, TString>& patches
) noexcept {
    try {
        TVector<std::pair<TString, TString>> files;

        for (auto& inputFile : itsConfigPaths) {

            if (!TFsPath(inputFile).Exists()) {
                WARNING_LOG << "File does not exist: " << inputFile << Endl;
                continue;
            }

            TFileInput file(inputFile);
            NJson::TJsonValue json;
            if (!NJson::ReadJsonTree(&file, &json, false)) {
                WARNING_LOG << "Failed to parse JSON: " << file.ReadAll() << Endl;
                continue;
            }

            if (json.Has("files")) {
                const auto& filesJson = json["files"];
                for (const auto& f : filesJson.GetArraySafe()) {
                    const auto& name = f["name"].GetString();
                    if (CheckFileName(name)) {
                        TStringStream value;
                        for (const auto &i: f["value"].GetMap()) {
                            DumpJsonItem(value, i.first, i.second);
                        }
                        files.emplace_back(name, value.Str());
                    }
                }
            }

            // first apply patches from robots, then from humans
            HandleItsPatches(json, patches, "robotPatches");
            HandleItsPatches(json, patches, "patches");
        }

        for (const auto& f : files) {
            TFileOutput output(JoinFsPaths(dstFolder, f.first));
            output << f.second;
        }
    } catch (...) {
        INFO_LOG << "Failed to parse ITS config: " << CurrentExceptionMessage() << Endl;
    }
}
