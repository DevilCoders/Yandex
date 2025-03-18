#include "config.h"

#include <library/cpp/getopt/last_getopt.h>

using namespace NReverseGeocoder::NGenerator;

TConfig::TConfig(int argc, const char* argv[]) {
    NLastGetopt::TOpts options;
    {
        options
            .AddLongOption('i', "input", "-- input GeoBase protobuf data path; maybe comma-separated list; maybe dirname for autoselect all files")
            .Required()
            .RequiredArgument("PATH")
            .StoreResult(&InputPath);
        options
            .AddLongOption('o', "output", "-- output geo-data path")
            .Required()
            .RequiredArgument("PATH")
            .StoreResult(&OutputPath);
        options
            .AddLongOption('e', "exclude-ids-list", "-- file with list of regions' id, which must be excluded from result")
            .Optional()
            .RequiredArgument("EXCLUDE_LIST")
            .StoreResult(&ExcludePath);
        options
            .AddLongOption('s', "subst-ids-list", "-- file with list of regions' id substitutions; $line := $from \\t $to")
            .Optional()
            .RequiredArgument("EXCLUDE_LIST")
            .StoreResult(&SubstPath);
        options
            .AddLongOption("save-raw-borders", "-- save raw borders in geo-data")
            .NoArgument()
            .Optional()
            .DefaultValue("0")
            .OptionalValue("1")
            .StoreResult(&SaveRawBorders);
        options.AddHelpOption('h');
    }
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);
}
