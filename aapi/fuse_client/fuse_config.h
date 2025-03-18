#pragma once

#include <util/system/platform.h>

#define FUSE_USE_VERSION 29

#if defined(_linux_)
#   include <contrib/libs/fuse/include/fuse.h>
#   include <contrib/libs/fuse/include/fuse_lowlevel.h>
#elif defined(_darwin_)
#   include <contrib/libs/osxfuse/include/fuse.h>
#   include <contrib/libs/osxfuse/include/fuse_lowlevel.h>
#endif
