#pragma once

#include <tools/snipmake/argv/opt.h>

#include <util/generic/string.h>

namespace NHebe {
    class TConfig {
    public:
        TConfig() {
        }

        TConfig(int argc, const char* argv[]) {
            using namespace NLastGetopt;
            TOpts opt;
            TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);
            TOpt& s = opt.AddCharOption('s', REQUIRED_ARGUMENT, "mapreduce server");
            TOpt& u = opt.AddCharOption('u', REQUIRED_ARGUMENT, "mapreduce user");
            TOpt& keepInput = opt.AddLongOption("keep-input", "don't drop final/ tables").HasArg(NO_ARGUMENT);
            TOpt& dropResult = opt.AddLongOption("drop-result", "drop resulting aggregated table, makes sense only with --keep-input").HasArg(NO_ARGUMENT);
            TOpt& pre = opt.AddLongOption("mr-prefix", "mr tables prefix, lalala/ under which final/ is located and where \"aggregated\" will be placed").HasArg(REQUIRED_ARGUMENT);
            TOpt& maxAge = opt.AddLongOption("max-record-age", "drop records older than that much seconds(time_t)").HasArg(REQUIRED_ARGUMENT);
            TOpt& format = opt.AddLongOption("format", "export format, supported: tstab, protobin").HasArg(REQUIRED_ARGUMENT);
            opt.SetFreeArgsNum(0);

            TOptsParseResult o(&opt, argc, argv);

            Version = Has_(opt, o, &v);
            MRServer = GetOrElse_(opt, o, &s, "");
            MRUser = GetOrElse_(opt, o, &u, "");
            MRPrefix = GetOrElse_(opt, o, &pre, "");
            DropInput = !Has_(opt, o, &keepInput);
            DropResult = Has_(opt, o, &dropResult);
            MaxAge = FromString<ui32>(GetOrElse_(opt, o, &maxAge, "0"));
            Format = GetOrElse_(opt, o, &format, "");
        }

        bool Version = false;
        TString MRServer;
        TString MRUser;
        TString MRPrefix;
        TString OutputName = "aggregated"; //without prefix
        bool DropInput = true;
        bool DropResult = false;
        ui32 MaxAge = 0;
        TString Format;
    };
}
