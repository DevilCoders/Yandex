#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/reverse_geocoder/generator/main.h>
#include <library/cpp/reverse_geocoder/core/reverse_geocoder.h>
#include <library/cpp/reverse_geocoder/core/geo_data/debug.h>
#include <library/cpp/reverse_geocoder/open_street_map/converter.h>
#include <library/cpp/reverse_geocoder/yandex_map/converter.h>
#include <library/cpp/reverse_geocoder/logger/log.h>

#include <library/cpp/reverse_geocoder/tool/border/main.h>
#include <library/cpp/reverse_geocoder/tool/lookup/lookup.h>
#include <library/cpp/reverse_geocoder/tool/benchmark/benchmark.h>
#include <library/cpp/reverse_geocoder/tool/stat/stat.h>
#include <library/cpp/reverse_geocoder/tool/geodata4/convert.h>
#include <library/cpp/reverse_geocoder/tool/tsv/convert.h>

using namespace NReverseGeocoder;

static int main_convert(int argc, const char* argv[]) {
    NLastGetopt::TOpts options;

    TString inputPath;
    options
        .AddLongOption('i', "input", "-- input data path")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&inputPath);

    TString outputPath;
    options
        .AddLongOption('o', "output", "-- output GeoBase protobuf data path")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&outputPath);

    ui16 jobsNumber;
    options
        .AddLongOption('j', "threads", "-- threads number")
        .Optional()
        .RequiredArgument("INTEGER")
        .DefaultValue(ToString(NReverseGeocoder::NOpenStreetMap::OptimalThreadsNumber()))
        .StoreResult(&jobsNumber);

    TString type;
    options
        .AddLongOption('t', "type", "-- input data type")
        .Optional()
        .RequiredArgument("OSM | geodata4 | YandexMap | tsv")
        .DefaultValue("OSM")
        .StoreResult(&type);

    options
        .AddLongOption("gz", "-- input is gzipped")
        .Optional()
        .NoArgument();

    TString skipKindList;
    options
        .AddLongOption('k', "skip_kind", "-- comma-separated list of toponym's kind")
        .Optional()
        .DefaultValue("street,house,hydro,vegetation,railway,route,station,other")
        .RequiredArgument("SKIPKIND")
        .StoreResult(&skipKindList);

    int countryId = 0;
    options
        .AddLongOption('c', "country_id", "-- check matching via geobase and 'geodata.bin' ('-g') for specified country; has precedence on '-m'")
        .Optional()
        .RequiredArgument("COUNTRY_ID")
        .StoreResult(&countryId);

    TString geoDataFileName;
    options
        .AddLongOption('g', "geodata", "-- path to standard 'geodata.bin'-file")
        .Optional()
        .DefaultValue("/var/cache/geobase/geodata4.bin")
        .RequiredArgument("GEODATA")
        .StoreResult(&geoDataFileName);

    TString geoMappingFileName;
    options
        .AddLongOption('m', "geomapping_data", "-- file with mapping of geobase id into geocoder::toponym names; ignored if '-c' specified")
        .Optional()
        .RequiredArgument("PATH")
        .StoreResult(&geoMappingFileName);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    if (type == "OSM") {
        NReverseGeocoder::NOpenStreetMap::RunPoolConvert(inputPath.c_str(), outputPath.c_str(), jobsNumber);
    } else if (type == "geodata4") {
        ConvertGeoData4(inputPath.c_str(), outputPath.c_str());
    } else if (type == "YandexMap") {
        NReverseGeocoder::NYandexMap::TConfig config;
        {
            config.Gz = optParseResult.Has("gz");
            config.Jobs = jobsNumber;
            config.SkipKindList = skipKindList;
            config.CountryId = countryId;
            config.GeoDataFileName = geoDataFileName;
            config.GeoMappingFileName = geoMappingFileName;
        };
        NReverseGeocoder::NYandexMap::Convert(inputPath.c_str(), outputPath.c_str(), config);
    } else if (type == "tsv") {
        ConvertTsv(inputPath, outputPath);
    } else {
        ythrow yexception() << "Unknown type: " << type;
    }

    return EXIT_SUCCESS;
}

static int main_benchmark(int argc, const char* argv[]) {
    NLastGetopt::TOpts options;

    TString base;
    options
        .AddLongOption("base", "-- GeoBase data")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&base);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    RunBenchmark(base.c_str());

    return EXIT_SUCCESS;
}

static int main_lookup(int argc, const char* argv[]) {
    NLastGetopt::TOpts options;

    TString base;
    options
        .AddLongOption("base", "-- GeoBase data")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&base);

    TString pointsDataFile("-");
    options
        .AddLongOption("points", "-- file with points (in format 'lat lon' pairs in each pow); used '-' (by default) for console input")
        .Optional()
        .RequiredArgument("PATH")
        .StoreResult(&pointsDataFile);

    options
        .AddLongOption("trace", "-- trace parents from region tree")
        .NoArgument()
        .Optional();

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    TLookupTool::TConfig config;
    config.Trace = optParseResult.Has("trace");
    config.Path = base;

    return TLookupTool(config).Run(pointsDataFile);
}

static int main_show(int argc, const char* argv[]) {
    NLastGetopt::TOpts options;

    TString base;
    options
        .AddLongOption("base", "-- GeoBase data")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&base);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    NReverseGeocoder::TReverseGeocoder reverseGeocoder(base.c_str());
    NReverseGeocoder::NGeoData::Show(Cout, reverseGeocoder.GeoData());

    return EXIT_SUCCESS;
}

static int main_stat(int argc, const char* argv[]) {
    NLastGetopt::TOpts options;

    TString base;
    options
        .AddLongOption("base", "-- GeoBase data")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&base);

    ui16 topMemoryRegions;
    options
        .AddLongOption("top-memory-regions")
        .Optional()
        .RequiredArgument("INTEGER")
        .DefaultValue("10")
        .StoreResult(&topMemoryRegions);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    ShowGeoDataStat(base.c_str(), topMemoryRegions);

    return EXIT_SUCCESS;
}

int main(int argc, const char* argv[]) {
    InitGlobalLog2Console(TLOG_DEBUG);

    TModChooser modChooser;
    {
        modChooser.AddMode("generate", NGenerator::main, "-- generate base");
        modChooser.AddMode("border", NBorderExtractor::main, "-- border extractor");
    }

    modChooser.AddMode(
        "convert",
        main_convert,
        "-- convert to reverse-geocoder format");

    modChooser.AddMode(
        "benchmark",
        main_benchmark,
        "-- run benchmark");

    modChooser.AddMode(
        "lookup",
        main_lookup,
        "-- lookup RegionId by location");

    modChooser.AddMode(
        "show",
        main_show,
        "-- show GeoData meta info");

    modChooser.AddMode(
        "stat",
        main_stat,
        "-- show GeoData statistics");

    return modChooser.Run(argc, argv);
}
