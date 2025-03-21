#include "public.h"

#include <cloud/storage/core/protos/media.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

bool IsDiskRegistryMediaKind(NProto::EStorageMediaKind mediaKind);
TString MediaKindToString(NProto::EStorageMediaKind mediaKind);
TString MediaKindToComputeType(NProto::EStorageMediaKind mediaKind);
bool ParseMediaKind(const TStringBuf s, NProto::EStorageMediaKind* mediaKind);

}   // namespace NCloud
