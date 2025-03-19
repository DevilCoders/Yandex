#pragma once

#include "public.h"

#include <cloud/filestore/libs/service/filestore.h>

#include <util/datetime/base.h>

struct stat;
struct statfs;

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

int ErrnoFromError(ui32 code);

void ConvertAttr(const NProto::TNodeAttr& attr, struct stat& st);
void ConvertStat(const NProto::TFileStore& info, struct statfs& st);

inline timespec TimeSpecFromMicroSeconds(ui64 us)
{
    timespec time = {};
    time.tv_sec = us / 1000000;
    time.tv_nsec = (us % 1000000) * 1000;
    return time;
}

inline ui64 TimeSpecToMicroSeconds(const timespec& time)
{
    return static_cast<ui64>(time.tv_sec) * 1000000 + time.tv_nsec / 1000;
}

}   // namespace NCloud::NFileStore::NGateway
