#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/content/description.h>
#include <nginx/modules/strm_packager/src/fbs/description.fbs.h>

#include <library/cpp/json/json_value.h>

namespace NStrm::NPackager {
    // Convert json description to our internal flatbuffers representation
    TBuffer ParseVodDescription(TRequestWorker& request, const TBuffer& buffer);
}
