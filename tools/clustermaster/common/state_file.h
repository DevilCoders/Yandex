#pragma once

#include <util/folder/path.h>

TFsPath StateFilePath(const TFsPath& varDir, const TString& targetName);
TFsPath StateFilePathEx(const TFsPath& varDir, const TString& targetName);
TFsPath VariablesFilePath(const TFsPath& varDir);
TFsPath MasterVariablesDeprecatedFilePath(const TFsPath& varDir);
TFsPath WorkerVariablesDeprecatedFilePath(const TFsPath& varDir);
