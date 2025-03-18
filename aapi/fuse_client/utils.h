#pragma once

#include <aapi/lib/common/object_types.h>

#include <util/system/fstat.h>


namespace NAapi {

int utimensat_impl(const char* path, const struct timespec tv[2]);

TFileStat CustomModeStats(mode_t mode, time_t start, ui64 size);
TFileStat DefaultModeStats(EEntryMode mode, time_t start, ui64 size);

}
