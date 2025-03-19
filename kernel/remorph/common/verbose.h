#pragma once

#include <util/stream/trace.h>
#include <util/system/defaults.h>
#include <util/generic/string.h>

void SetVerbosityLevel(int level);
int GetVerbosityLevel();

IOutputStream& GetInfoStream();

int VerbosityLevelFromString(const TString& str);

#define REPORT(level, args) \
    do                                                                    \
        if (Y_UNLIKELY(int(TRACE_##level) <= ::GetVerbosityLevel())) {  \
            ::GetInfoStream() << args << Endl;                            \
        }                                                                 \
    while(false)
