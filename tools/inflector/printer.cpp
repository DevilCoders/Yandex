#include "printer.h"

namespace NInflector {

TPrinter::TPrinter(IOutputStream& output, bool verbose)
    : Output(output)
    , Verbose(verbose)
{
}

void TPrinter::Print(const TUtf16String& text, const NInfl::TSimpleResultInfo& resultInfo) const {
    Output << text;
    if (Verbose) {
        Output << "\t" << resultInfo.Grams;
        Output << Endl;
        Output << "" << resultInfo.Debug;
    }
    Output << Endl;
}

} // NInflector
