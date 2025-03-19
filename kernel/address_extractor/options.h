#pragma once

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/langmask/langmask.h>

class TExtractorOpts {
public:
    bool OnlyWithNumbers;
    bool OnlyWithCity;
    bool CombineWithNextInLine;
    bool CombineWithNext;
    bool CombineWithTheOnlyCity;
    bool CombineWithHeader;
    bool DebugFacts;
    bool RemoveSmaller;
    TLangMask Lang;
    ECharset Encoding;
    size_t MaxFactNumber;
    bool DoAutoPrintResult;


public:
    TExtractorOpts()
        : OnlyWithNumbers(false)
        , OnlyWithCity(false)
        , CombineWithNextInLine(false)
        , CombineWithNext(false)
        , CombineWithTheOnlyCity(false)
        , CombineWithHeader(false)
        , DebugFacts(false)
        , RemoveSmaller(false)
        , Encoding(CODES_UNKNOWN)
        , MaxFactNumber(200)
        , DoAutoPrintResult(true)
    {

    }
};

class TRunOpts {
private:
    NLastGetopt::TOpts Options;

private:
    void InitOptParser();
    void Parse(int argc, char* argv[]);

public:
    TString GztPath;
    TString FactsPath;

    bool WaitEndSign;
    bool PrintEndSign;
    TString InputEndSign;
    TString OutputEndSign;

    TExtractorOpts ExtractorOpts;

    size_t ThreadsCount;

public:
    TRunOpts(int argc, char* argv[]);
};
