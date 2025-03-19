#include "tools.h"
#include <util/string/printf.h>
#include <util/datetime/base.h>
#include <util/folder/path.h>

#if !defined(_win_)
#   include <sys/resource.h>
#endif

namespace NMango
{
    TString ExtractType(const TString &fullname)
    {
        TVector<TString> parts;
        StringSplitter(fullname).Split('.').SkipEmpty().Limit(2).Collect(&parts);
        parts[0].to_lower();
        return parts[0];
    }

    Pire::Scanner RegexpCompile(const char* pattern)
    {
        std::vector<wchar32> ucs4;
        Pire::Encodings::Utf8().FromLocal(pattern, pattern + strlen(pattern), std::back_inserter(ucs4));

        return Pire::Lexer(ucs4.begin(), ucs4.end())
            .AddFeature(Pire::Features::CaseInsensitive())
            .SetEncoding(Pire::Encodings::Utf8())
            .Parse()
            .Surround()
            .Compile<Pire::Scanner>();
    }

    bool RegexpMatches(const Pire::Scanner& scanner, const char* ptr, size_t len)
    {
        return Pire::Runner(scanner)
            .Begin()
            .Run(ptr, len)
            .End();
    }

    TString ExtractFileName(const TString &fullName)
    {
        return TFsPath(fullName).GetName();
    }

    long int GetMemoryUsage()
    {
#if defined(_win_)
        return 0;
#else
        int who = RUSAGE_SELF;
        struct rusage usage;
        getrusage(who, &usage);
        return usage.ru_maxrss;
#endif
    }

    void ShitToStack()
    {
        char array[4000000];
        memset(array, 123, sizeof array);
    }

    TString GetTimeFormatted(const char* format /*= "%Y%m%dT%H%M%S"*/)
    {
        struct tm tm;
        return Strftime(format, TInstant::Now().LocalTime(&tm));
    }

    void RemoveLeadingZeroesOfNumberPrefix(TString &s)
    {
        if (s.empty())
            return;
        size_t len = 0;
        while (len < s.length() && s[len] == '0')
            ++len;
        if (len == s.length() || len > 0 && (s[len] < '0' || s[len] > '9'))
            --len;
        s = s.substr(len);
    }
}

