#pragma once

#include <kernel/inflectorlib/phrase/simple/simple.h>

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NInflector {

class TPrinter {
    IOutputStream& Output;
    bool Verbose;
    TWtringBuf Delimiter;

public:
    TPrinter(IOutputStream& output, bool verbose);

    void Print(const TUtf16String& text, const NInfl::TSimpleResultInfo& resultInfo) const;
};

} // NInflector
