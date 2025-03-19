#pragma once

#include "public.h"

#include <cloud/filestore/public/api/protos/fs.pb.h>

#include <ydb/core/protos/filestore_config.pb.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

void Convert(const NKikimrFileStore::TConfig& config, NProto::TFileStore& fileStore);

}   // namespace NCloud::NFileStore::NStorage
