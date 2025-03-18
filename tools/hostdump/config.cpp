#include "config.h"

#include <library/cpp/svnversion/svnversion.h>



TFormatHostDumpConfig::TFormatHostDumpConfig(int argc, const char** argv) {
    Init();

    Opts.AddLongOption('v', "version",          "print version").NoArgument();
    Opts.AddLongOption('d', "dir",           "root dir for output dump")
        .StoreResult(&RootDir).DefaultValue(RootDir).RequiredArgument();

    Opts.AddHelpOption();
    ParseResult.Reset(new NLastGetopt::TOptsParseResult(&Opts, argc, argv));

    if (ParseResult->Has('v')) {
        Cerr << PROGRAM_VERSION << Endl;
        exit(1);
    }
}

void TFormatHostDumpConfig::DumpConfig(IOutputStream& out) const {
    out << "RootDir: " << RootDir << Endl;
}

void TFormatHostDumpConfig::Init() {
}
