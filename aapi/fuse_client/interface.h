#pragma once

#include "fuse_config.h"
#include "file_system.h"

#include <aapi/lib/common/object_types.h>

#include <util/generic/string.h>

fuse_operations GetAapiFuseOperations();
fuse_args CreateFuseArgs(const TVector<TString> args);

THolder<NAapi::TFileSystemHolder> InitSvn(const NAapi::TPathInfo& path, const NAapi::TSvnCommitInfo& commitInfo, const TString& discStore, const TString& proxyAddr);
THolder<NAapi::TFileSystemHolder> InitHg(const NAapi::TPathInfo& path, const NAapi::THgChangesetInfo& commitInfo, const TString& discStore, const TString& proxyAddr);
