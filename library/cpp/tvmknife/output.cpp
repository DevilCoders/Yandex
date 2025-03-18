#include "output.h"

#include <util/stream/null.h>

namespace NTvmknife::NOutput {
    static bool QUITE = true;

    TOutStream::TOutStream(IOutputStream& stream)
        : Stream(stream)
        , Color(true)
    {
        Stream << Color.DarkYellow();
    }

    TOutStream::~TOutStream() {
        Stream << Color.OldColor();
    }

    TOutStream Out() {
        return TOutStream(QUITE ? Cnull : Cerr);
    }

    bool& GetQuiteFlag() {
        return QUITE;
    }
}
