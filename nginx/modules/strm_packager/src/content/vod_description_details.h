#pragma once

#include <nginx/modules/strm_packager/src/content/description.h>
#include <nginx/modules/strm_packager/src/fbs/description.fbs.h>

namespace NStrm::NPackager::NVodDescriptionDetails {
    void ParseV1VodDescription(const TString& jsonString, flatbuffers::FlatBufferBuilder& builder);
    void ParseV2VodDescription(const TString& jsonString, flatbuffers::FlatBufferBuilder& builder);
}
