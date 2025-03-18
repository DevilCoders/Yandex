#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/source_convert.h>

namespace NStrm::NPackager {

    TSourceFuture ConvertSubtitlesRawToTTML(TRequestWorker& request, TSourceFuture source);

}
