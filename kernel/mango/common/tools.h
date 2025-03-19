#pragma once

#include "protos.h"
#include <library/cpp/regex/pire/pire.h>
#include <util/generic/ymath.h>

namespace NMango
{
    TString ExtractType(const TString &fullname);

    Pire::Scanner RegexpCompile(const char* pattern);
    bool RegexpMatches(const Pire::Scanner& scanner, const char* ptr, size_t len);


    TString ExtractFileName(const TString &fullName);

    long int GetMemoryUsage();

    void ShitToStack();

    TString GetTimeFormatted(const char* format = "%Y%m%dT%H%M%S");

    inline float CalcIdf(float freq, float quotesCount)
    {
        return log(quotesCount / freq);
    }

    void RemoveLeadingZeroesOfNumberPrefix(TString &s);
}
