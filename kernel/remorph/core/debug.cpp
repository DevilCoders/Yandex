#include <util/stream/debug.h>
#include <util/stream/null.h>
#include <util/stream/str.h>
#include "debug.h"

namespace NPrivate {

class TPrefixedLineOutput: public IOutputStream {
public:
    TPrefixedLineOutput(IOutputStream* slave, const TString& prefix)
        : Slave(slave)
        , Prefix(prefix)
        , Buffer(prefix) {
    }

private:
    void DoWrite(const void* buf_, size_t len_) override {
        const char* nl = nullptr;
        const char* buf = (const char*)buf_;
        size_t len = len_;
        TStringOutput sOut(Buffer);
        for (nl = (const char*)memchr(buf, '\n', len); nl; nl = (const char*)memchr(buf, '\n', len)) {
            size_t l2write = nl - buf;
            sOut.Write(buf, l2write);
            *Slave << Buffer << Endl;
            Buffer = Prefix;
            buf = nl + 1;
            len = len - l2write - 1;
        }
        sOut.Write(buf, len);
    }

private:
    IOutputStream* Slave;
    TString Prefix;
    TString Buffer;
};

} // NPrivate

namespace NRemorph {

unsigned debugFlags = 0;

unsigned SetDebugOutFlags(unsigned or_ed_flags) {
    unsigned retval = debugFlags;
    debugFlags = or_ed_flags;
    return retval;
}

#define X(A, B) IOutputStream& GetDebugOut##A() {                     \
        static NPrivate::TPrefixedLineOutput out_(&Cdbg, #A": ");  \
        IOutputStream& out = out_;                                    \
        return (DO_##A & debugFlags) ? out : Cnull;                   \
    }
NWRE_DEBUG_OUT_FLAGS
#undef X

} // NRemorph
