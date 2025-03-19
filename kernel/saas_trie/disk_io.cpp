#include "disk_io.h"

#include <util/folder/dirut.h>
#include <util/stream/file.h>

namespace NSaasTrie {
    TBlob TDiskIO::Map(const TString& path) const {
        if (!NFs::Exists(path)) {
            return {};
        }
        return TBlob::FromFile(path);
    }

    THolder<IOutputStream> TDiskIO::CreateWriter(const TString& path) const {
        auto out = MakeHolder<TFixedBufferFileOutput>(path, 1ul << 22);
        out->SetFinishPropagateMode(true);  // workaround silent changes like https://a.yandex-team.ru/arc/commit/3227438
        return out;
    }
}
