#include <library/cpp/archive/yarchive.h>

#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/memory/blob.h>
#include <util/stream/input.h>

namespace {
    const unsigned char data[] = {
#include "data.inc"
    };

    struct TData: public TArchiveReader {
        inline TData()
            : TArchiveReader(TBlob::NoCopy(data, sizeof(data)))
        {
        }
    };
}

TAutoPtr<IInputStream> OpenTDefaultDecimatorDataInputStream() {
    return Singleton<TData>()->ObjectByKey("/default_decimator.lst");
}
