#include "options.h"

#include <util/generic/string.h>
#include <util/stream/str.h>
#include <util/stream/file.h>

void TDaemonOptions::Parse(int argc, char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.AddHelpOption('?');
    opts.AddVersionOption('v');
    BindToOpts(opts);
    try {
        NLastGetopt::TOptsParseResultException res(&opts, argc, const_cast<const char**>(argv));
        if (res.GetFreeArgCount() < 1) {
            ythrow NLastGetopt::TUsageException() << "ERROR: missing CONFIG_FILE argument";
        }
        ConfigFileName = res.GetFreeArgs()[0];

        if (SecdistPath && SecdistService) {
            auto varPaths = SecdistVarsPaths;
            for (const auto& envPath: SecdistEnvPaths) {
                TFileInput fi(envPath);
                TString line;
                while (fi.ReadLine(line)) {
                    TStringBuf k, v;
                    TStringBuf(line).Split("=", k, v);
                    if (k && v) {
                        varPaths[TString(k)] = TString(v);
                    }
                }
            }
            Preprocessor.ReadSecdist(SecdistPath, SecdistService, SecdistDbName, varPaths);
        }

    } catch (NLastGetopt::TUsageException& e) {
        // append usage info to TUsageExceptions, other exceptions go through as-is
        TString usage;
        TStringOutput usageStream(usage);
        opts.PrintUsage(argv[0], usageStream);
        e << "\n" << usage;
        throw;
    }
}

void TDaemonOptions::BindToOpts(NLastGetopt::TOpts& opts) {
    opts.AddCharOption('t', "test validate config file and exit")
        .StoreTrue(&ValidateOnly).DefaultValue(ValidateOnly ? "true" : "false");
    opts.AddCharOption('V', "set config Lua variable key to value").RequiredArgument("key=value")
        .KVHandler([this](TString key, TString value){ this->Preprocessor.SetVariable(key, value); });
    opts.AddCharOption('P', "patch config field key to value").RequiredArgument("key=value")
        .KVHandler([this](TString key, TString value){ this->Preprocessor.AddPatch(key, value); });
    opts.AddCharOption('E', "read variables from file").RequiredArgument("filename")
        .Handler1T<TString>([this](TString filename){ this->Preprocessor.ReadEnvironment(filename); });
    opts.AddLongOption("its", "path to ITS config").RequiredArgument("filename")
        .Handler1T<TString>([this](TString filename){ this->Preprocessor.SetItsConfigPath(filename); });

    opts.AddLongOption("secdist", "path to secdist config").RequiredArgument("filename")
        .StoreResult<TString>(&SecdistPath);
    opts.AddLongOption("secdist-service", "secdist service name").RequiredArgument("servicename")
        .StoreResult<TString>(&SecdistService);
    opts.AddLongOption("secdist-dbname", "secdist postgres database name").RequiredArgument("dbname")
            .StoreResult<TString>(&SecdistDbName);
    opts.AddLongOption("secdist-env", "mapping Lua variables by json paths from secdist").RequiredArgument("path")
        .InsertTo<TString>(&SecdistEnvPaths);
    opts.AddCharOption('S', "set config Lua variable by json path from secdist").RequiredArgument("key=value")
            .KVHandler([this](TString key, TString value){ this->SecdistVarsPaths[key] = value; });

    opts.SetFreeArgsMin(1);
    opts.SetFreeArgTitle(0, "CONFIG_FILE", "path to config file");
}
