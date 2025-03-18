#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <library/cpp/getopt/last_getopt.h>

class TFormatHostDumpConfig {
public:
    TFormatHostDumpConfig(int argc, const char** argv);
    ~TFormatHostDumpConfig() {}
    void DumpConfig(IOutputStream& out) const;
    TString RootDir;

private:
    void Init();

private:
    NLastGetopt::TOpts Opts;
    THolder<NLastGetopt::TOptsParseResult> ParseResult;
};
