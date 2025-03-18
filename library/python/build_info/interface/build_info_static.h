#pragma once

#include <library/cpp/build_info/build_info.h>

#if defined(__cplusplus)
extern "C" {
#endif

const char* GetPyCompilerFlags(); // "-std=c++14 -DNDEBUG -O2 -m64 ..."
const char* GetPyBuildInfo();     // Compiler version and flags

#if defined(__cplusplus)
}
#endif

