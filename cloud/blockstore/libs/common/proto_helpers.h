#pragma once

#include "public.h"

#include <cloud/blockstore/public/api/protos/volume.pb.h>

#include <google/protobuf/message.h>

#include <util/generic/string.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

bool TryParseProtoTextFromStringWithoutError(
    const TString& text,
    google::protobuf::Message& dst);

bool TryParseProtoTextFromString(
    const TString& text,
    google::protobuf::Message& dst);

bool TryParseProtoTextFromFile(
    const TString& fileName,
    google::protobuf::Message& dst);

bool IsReadWriteMode(const NProto::EVolumeAccessMode mode);

}   // namespace NCloud::NBlockStore
