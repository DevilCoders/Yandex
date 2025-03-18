#include "rpslogger.h"

#include <util/stream/format.h>


TRpsLogger::TRpsLogger(const TString& fileName)
    : Out(fileName)
{}

void TRpsLogger::DumpStats(const TRpsScheduleIterator& it) {
    auto now = Now();
    if ((now - Last) > TDuration::MilliSeconds(500)) {
        Out << Prec(now.SecondsFloat(), PREC_POINT_DIGITS, 3) << "\t" << it.GetRps() << "\t" << 1 << Endl;
        Last = now;
    }
}
