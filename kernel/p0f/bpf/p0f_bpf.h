#pragma once

#define SFX(prog_name, suffix) _binary_##prog_name##_##suffix
#define START(prog_name) SFX(prog_name, start)
#define END(prog_name) SFX(prog_name, end)
#define LENGTH(prog_name) SFX(prog_name, len)

#define EXTERN_RESOURCE(prog_name)                          \
    extern char START(prog_name)[];                         \
    extern char END(prog_name)[];                           \
    static inline size_t LENGTH(prog_name)() {              \
        return (size_t)(END(prog_name) - START(prog_name)); \
    }

EXTERN_RESOURCE(p0f_bpf)

#include "p0f_bpf_types.h"

#undef EXTERN_RESOURCE
#undef SIZE
#undef END
#undef START
#undef SFX
