#pragma once

#include <search/config/preprocessor/preprocessor.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>

#include <algorithm>

class TConfigPatcher {
public:
    TConfigPatcher(const TString& patchPrefix = {});

    static TString DetectDatacenterCode();
    TString ReadAndProcess(const TString & filename);
    void ReadEnvironment(const TString& filename, bool push = true);
    void ReadEnvironmentFromYaml(const TString& filename);
    void ReadEnvironmentPlain(const TString& filename);
    void ReadSecdist(const TString& filename, const TString& service, const TString& dbname, const THashMap<TString, TString>& varsPaths);
    void ReReadEnvironment();
    void SetVariable(const TStringBuf name, const TStringBuf value, const bool setEnv = false);
    void AddPatch(const TString& path, const TString& value);
    TConfigPreprocessor* GetPreprocessor() {
        return &Preprocessor;
    }
    void SetStrict(bool value = true) {
        Preprocessor.SetStrict(value);
    }
    void SetItsConfigPath(const TString& path) {
        ItsConfigPaths.push_back(path);
    }
    void SetItsConfigPath(const TVector<TString>& path) {
        ItsConfigPaths = path;
    }
    const TVector<TString>& GetItsConfigPath() const {
        return ItsConfigPaths;
    }
    bool ItsConfigPathExists() const {
        return std::any_of(ItsConfigPaths.begin(), ItsConfigPaths.end(), [](const auto& path){return path;});
    }

private:
    TConfigPreprocessor Preprocessor;
    THashMap<TString, TString> Patches;
    TString PatchPrefix;
    TVector<TString> EnvironmentFiles;
    TVector<TString> ItsConfigPaths;
};
