#include "tools.h"

#include <util/generic/ymath.h>
#include <util/generic/algorithm.h>

#include <util/stream/format.h>

namespace NTools {

TString PrintItemPerSecond(TDuration time, size_t itemscount) {
    TStringStream res;
    double seconds = (double)(time.MilliSeconds()) / 1000.0;
    if (itemscount == 0)
        res << "0B";
    else if (time == TDuration::Zero() && seconds == 0.0)
        res << "INF";
    else
        res << HumanReadableSize(itemscount / seconds, SF_BYTES);
    return res.Str();
}

TString PrintTimePerItem(TDuration time, size_t count) {
    TStringStream res;
    if (time == TDuration::Zero())
        res << "0";
    else if (count == 0)
        res << "INF";
    else
        res << HumanReadable(time / count);

    return res.Str();
}

static TString Summary(TInstant start, size_t totalLines, TInstant prev, size_t prevLines, bool shrinked = true) {
    TInstant current = Now();
    TStringStream res;

    if (shrinked)
        res << HumanReadableSize(totalLines, SF_QUANTITY);
    else
        res << totalLines;

    res << " processed (" << HumanReadable(current - start) << ", "
        << PrintTimePerItem(current - prev, prevLines) << " -> "
        << PrintTimePerItem(current - start, totalLines) << " avg)";

    return res.Str();
}

static TString SummaryBytes(TInstant start, size_t totalLines, TInstant prev, size_t prevLines, bool shrinked = true) {
    TInstant current = Now();

    const size_t KB = 1024;
    const size_t MB = 1024 * KB;

    TStringStream res;

    if (shrinked)
        res << HumanReadableSize(totalLines, SF_BYTES);
    else
        res << totalLines << "B";

    res << " processed (" << HumanReadable(current - start) << ", "
        << PrintTimePerItem(current - prev, prevLines/MB) << "/MB -> "
        << PrintTimePerItem(current - start, totalLines/MB) << "/MB, "
        << PrintItemPerSecond(current - start, totalLines) << "/s avg)";

    return res.Str();
}


TPerformanceCounterBase::TPerformanceCounterBase(const TString& name, size_t logFreq, bool suspend)
    : Name(name)
    , LogFreq(logFreq)
    , StartTime(TInstant::Now())
    , PrevTime(StartTime)
    , LastSuspend(TInstant::Zero())
    , Output(&Clog)
{
    if (suspend)
        LastSuspend = StartTime;
}

void TPerformanceCounterBase::PrintCustomInfo() const {
    TString custom = CustomInfo();
    if (!!custom)
        (*Output) << ", " << custom;
}

void TPerformanceCounterBase::DoPrint(bool finished, const size_t currentCounter) const {
    (*Output) << Name << ": " << Summary(StartTime, currentCounter, PrevTime, LogFreq, !finished);
    PrintCustomInfo();
    (*Output) << Endl;
}

void TPerformanceCounterBase::Print(const size_t currentCounter) const {
    DoPrint(false, currentCounter);
}

void TPerformanceCounterBase::Finish() const {
    if (const size_t currentCounter = GetCurrentCounter()) {
        (*Output) << "(Finished) ";
        DoPrint(true, currentCounter);
    }
}


void TBytePerformanceCounter::DoPrint(bool finished, const size_t currentCounter) const {
    (*Output) << Name << ": " << SummaryBytes(StartTime, currentCounter, PrevTime, LogFreq, !finished);
    PrintCustomInfo();
    (*Output) << Endl;
}


} //namespace NTools
