#include "raw_channel.h"

#include <util/folder/path.h>

#include <library/cpp/logger/log.h>

namespace NFuse {
    int ArcFuseMount(const TFsPath& mountpoint, const TRawChannelOptions& options, TLog log);

    void ArcFuseUnmount(const TFsPath& mountpoint, int fd, TLog log);
}
