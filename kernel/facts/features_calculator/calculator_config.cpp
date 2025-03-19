#include "calculator_config.h"

#include <util/folder/path.h>
#include <util/stream/file.h>

namespace NUnstructuredFeatures {
    const TString TConfig::CLASSIFIER_FILES_CONFIG_PATH = "files/classifier_files";

    TConfig::TConfig(const TString& dataPath, const TString& configName, size_t version)
            : Version(version)
            , DataPath(dataPath)
            , ConfigName(configName)
    {
        TString configFileName = GetConfigFileName();
        Y_ENSURE(TFsPath(configFileName).Exists(), "There isn't a file " + configFileName);

        const TString configJson = TFileInput(configFileName).ReadAll();
        NSc::TValue config = NSc::TValue::FromJsonThrow(configJson);
        for (const NSc::TValue& versionedConfig : config.GetArray()) {
            if (static_cast<size_t>(versionedConfig["version"].GetIntNumber()) != Version) {
                continue;
            }
            Config = versionedConfig;
            return;
        }
        ythrow yexception() << "ERROR: config version " << Version << " cannot be found";
    }

    float TConfig::GetGlobalParameter(const TString &key, float defaultValue) const {
        if (Config.Has(key)) {
            return static_cast<float>(Config[key].GetNumber());
        } else {
            Cerr << "Can't find parameter '" << key << "'; set default value=" << defaultValue << Endl;
            return defaultValue;
        }
    }

    TString TConfig::GetFilePath(const TString &key) const {
        TStringBuf localPath = Config.TrySelect(CLASSIFIER_FILES_CONFIG_PATH)[key].GetString();
        if (localPath.empty())
            return TString();
        return GetFullPath(localPath);
    }

    TString TConfig::GetPath(const TString &selector) const {
        TStringBuf path = Config.TrySelect(selector).GetString();
        if (path.empty())
            return TString();
        return GetFullPath(path);
    }

    TVector<TString> TConfig::GetUsedFiles() const {
        TSet<TString> classifierFileNameSet;
        for (const auto& kv : Config.TrySelect(CLASSIFIER_FILES_CONFIG_PATH).GetDict()) {
            classifierFileNameSet.insert(GetFullPath(kv.second));
        }
        return TVector<TString>(classifierFileNameSet.begin(), classifierFileNameSet.end());
    }

    THashMap<TString, TString> TConfig::GetUsedFilesDict() const {
        THashMap<TString, TString> classifierFileNames;
        for (const auto& kv : Config.TrySelect(CLASSIFIER_FILES_CONFIG_PATH).GetDict()) {
            classifierFileNames[kv.first] = GetFullPath(kv.second);
        }
        return classifierFileNames;
    }

    TString TConfig::GetFullPath(const TStringBuf& localPath) const {
        return DataPath + "/" + localPath;
    }

    TString TConfig::GetConfigFileName() const {
        return GetFullPath(ConfigName);
    }

    const TString& TConfig::GetDataPath() const {
        return DataPath;
    }
}
