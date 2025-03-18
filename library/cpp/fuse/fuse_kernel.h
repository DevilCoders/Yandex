#pragma once

#include <util/system/platform.h>

#include <cstdint>

#if defined(_darwin_)
#include <contrib/libs/macfuse-headers/fuse_kernel.h>
#else
#include <contrib/libs/linux-headers/linux/fuse.h>
#endif

#define FUSE_USE_VERSION 29
