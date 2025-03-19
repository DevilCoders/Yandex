#include "options.h"

#include <library/cpp/getopt/small/last_getopt.h>

namespace NCloud::NFileStore {

using namespace NLastGetopt;

const THashMap<TString, EClientType> StrToType = {
    {"write", EClientType::Write},
    {"read", EClientType::Read},
};

////////////////////////////////////////////////////////////////////////////////

void TOptions::Parse(int argc, char** argv)
{
    TOpts opts;
    opts.AddHelpOption();

    opts.AddLongOption(
        "type",
        "specify client type, read or write")
        .RequiredArgument("STR")
        .Required()
        .StoreResult(&TypeStr)
        .Handler0([this] {
            auto it = StrToType.find(TypeStr);
            if (it == StrToType.end()) {
                ythrow yexception() << "Unknown type of client";
            } else {
                Type = it->second;
            }
        });

    opts.AddLongOption("file", "path to file")
        .RequiredArgument("STR")
        .Required()
        .StoreResult(&FilePath);

    opts.AddLongOption("filesize", "size of file in GB")
        .RequiredArgument("NUM")
        .Required()
        .StoreResult(&FileSize);

    opts.AddLongOption(
        "request-size",
        "specify request size in MB")
        .RequiredArgument("NUM")
        .StoreResult(&RequestSize)
        .DefaultValue(1);

    opts.AddLongOption(
        "sleep-between-writes",
        "specify time to sleep between writes in minutes")
        .RequiredArgument("NUM")
        .StoreResult(&SleepBetweenWrites)
        .DefaultValue(0);

    opts.AddLongOption(
        "sleep-before-start",
        "specify time to sleep before load starts")
        .RequiredArgument("NUM")
        .StoreResult(&SleepBeforeStart)
        .DefaultValue(0);


    TOptsParseResultException(&opts, argc, argv);
}

}   // namespace NCloud::NFileStore
