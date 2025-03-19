#include "artifacts_registry.h"
#include <library/cpp/resource/resource.h>
#include <library/cpp/protobuf/util/pb_io.h>
#include <util/stream/mem.h>

using namespace NRelevArtifacts;

TRegistryFormat NRelevArtifacts::GetWebSearchRegistry() {
    TRegistryFormat res;
    TString data = NResource::Find("/kernel/web_relev_artifacts/artifacts_registry.proto.txt");
    TMemoryInput stream(data.begin(), data.size());
    ParseFromTextFormat(stream, res);
    return res;
}
