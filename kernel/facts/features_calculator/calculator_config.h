#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NUnstructuredFeatures {
    class TConfig {
    public:
        TConfig(const TString& dataPath, const TString& configName, size_t version);
        TString GetConfigFileName() const;

        float GetGlobalParameter(const TString &key, float defaultValue = 0.0f) const;
        TString GetPath(const TString &selector) const;
        TString GetFilePath(const TString &key) const;
        TVector<TString> GetUsedFiles() const;
        THashMap<TString, TString> GetUsedFilesDict() const;
        const TString& GetDataPath() const;

    private:
        TString GetFullPath(const TStringBuf& localPath) const;

        size_t Version;
        NSc::TValue Config;

        const TString DataPath;
        const TString ConfigName;

        static const TString CLASSIFIER_FILES_CONFIG_PATH;
    };
}
