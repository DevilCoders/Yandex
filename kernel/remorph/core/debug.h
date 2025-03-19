#pragma once

class IOutputStream;

namespace NRemorph {

#define NWRE_DEBUG_OUT_FLAGS  \
    X(PARSER, 0)              \
    X(COMPILER, 1)            \
    X(CONVERTER_STORE, 2)     \
    X(CONVERTER_CLOSURE, 3)   \
    X(CONVERTER_ADD, 4)       \
    X(CONVERTER_FIND, 5)      \
    X(CONVERTER_CONVERT, 6)   \
    X(CONVERTER_NI, 7)        \
    X(EXECUTE, 8)             \
    X(EXECUTE_NFA, 9)         \
    X(MATCH, 10)

enum EDebugOutFlags {
#define X(A, B) DO_##A = (1 << B),
NWRE_DEBUG_OUT_FLAGS
#undef X
};

#define X(A, B) IOutputStream& GetDebugOut##A();
NWRE_DEBUG_OUT_FLAGS
#undef X

unsigned SetDebugOutFlags(unsigned or_ed_flags);

class TDBGOFlagsGuard {
private:
    unsigned OldFlags;
public:
    TDBGOFlagsGuard(unsigned flags)
        : OldFlags(SetDebugOutFlags(flags)) {
    }
    ~TDBGOFlagsGuard() {
        SetDebugOutFlags(OldFlags);
    }
};

} // NRemorph

#define NWRE_ENABLE_DEBUG_OUT 0

#if NWRE_ENABLE_DEBUG_OUT

#define NWRED(A) do { A; } while (false)
#define NWRE_UNUSED(T)

#else

#define NWRED(A) do {} while (false)
#define NWRE_UNUSED(T) Y_UNUSED(T)

#endif
