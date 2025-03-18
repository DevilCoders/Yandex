#include "library/python/build_info/interface/build_info_static.h"

#include <library/python/build_info/py2/buildinfo_data.h>

extern "C" const char* GetPyCompilerFlags() {
#if defined(BUILD_COMPILER_FLAGS)
    return BUILD_COMPILER_FLAGS;
#else
    return "";
#endif
}

extern "C" const char* GetPyBuildInfo() {
#if defined(BUILD_INFO)
    return BUILD_INFO;
#else
    return "";
#endif
}

