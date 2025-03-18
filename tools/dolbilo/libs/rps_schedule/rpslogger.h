#pragma once

#include <util/datetime/base.h>
#include <util/stream/file.h>

#include "rpsschedule.h"

class IOutputStream;

// Helper class to output current shooting state to log.
// This log can be used for futher analysis
class TRpsLogger {
public:
    TRpsLogger(const TString& fileName);
    void DumpStats(const TRpsScheduleIterator& it);

private:
    TFixedBufferFileOutput Out;
    TInstant Last;
};
