#include "state_file.h"

TFsPath StateFilePath(const TFsPath& varDir, const TString& targetName) {
    return varDir.Child("state").Child(targetName);
}

TFsPath StateFilePathEx(const TFsPath& varDir, const TString& targetName) {
    return StateFilePath(varDir, targetName).GetPath() + ".ex";
}

TFsPath VariablesFilePath(const TFsPath& varDir) {
    return varDir.Child("vars.version2");
}

TFsPath MasterVariablesDeprecatedFilePath(const TFsPath& varDir) {
    return varDir.Child("vars");
}

TFsPath WorkerVariablesDeprecatedFilePath(const TFsPath& varDir) {
    return varDir.Child("variables");
}
