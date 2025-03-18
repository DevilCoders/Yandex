#include "load.h"
#include "config.h"
#include "usage.h"

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/string.h>
#include <util/string/builder.h>
#include <util/generic/serialized_enum.h>

namespace NProtoConfig {
    namespace {
        void RegisterHelpHandler(NLastGetopt::TOpts& opts, NProtoBuf::Message& message) {
            auto handler = [&](const NLastGetopt::TOptsParser* parser) {
                parser->PrintUsage(Cerr);
                Cerr << Endl;
                PrintConfigUsage(message, Cerr, true);

                exit(0);
            };

            opts
                .AddLongOption('h', "help", "Print help and exit.")
                .Optional()
                .NoArgument()
                .Handler1(handler);
        }

    }


    void GetOpt(int argc, const char* argv[], NProtoBuf::Message& config, const TString& resource) {
        TLoadConfigOptions options;
        options.Resource = resource;
        GetOpt(argc, argv, config, options);
    }

    void GetOpt(int argc, const char* argv[], NProtoBuf::Message& config, TLoadConfigOptions& options) {
        NLastGetopt::TOpts opts;

        opts
            .AddLongOption('r', "resource", "resource name key for NResource::Find")
            .Optional()
            .DefaultValue(options.Resource)
            .RequiredArgument("RESOURCE")
            .StoreResult(&options.Resource);

        opts
            .AddLongOption('c', "config", "Config path.")
            .Optional()
            .RequiredArgument("PATH")
            .StoreResult(&options.Path);

        opts
            .AddLongOption("format", TStringBuilder() << "ConfigFormat: " << GetEnumAllNames<TLoadConfigOptions::EConfigFormat>())
            .Optional()
            .RequiredArgument("TLoadConfigOptions::EConfigFormat")
            .DefaultValue(options.ConfigFormat)
            .StoreResult(&options.ConfigFormat);

        opts
            .AddLongOption('V', "override", "Override config option. syntax: field=value or field.subfield=value")
            .Optional()
            .RequiredArgument("KEY=VAL")
            .AppendTo(&options.Overrides);

        RegisterHelpHandler(opts, config);
        NLastGetopt::TOptsParseResult(&opts, argc, argv);

        Y_ENSURE_EX(
            !options.Resource.empty() || !options.Path.empty(),
            NLastGetopt::TUsageException() << "Expected at least one of --resource/--config to be non-empty but " << (options.Resource.empty() ? "resource" : "config") << " is empty" );

        LoadWithOptions(config, options);
    }

    void LoadWithOptions(NProtoBuf::Message& config, const TLoadConfigOptions& options) {
        TString resourceDataHolder;
        THolder<IInputStream> configInputStream = nullptr;
        if (!options.Path.empty()) {
            configInputStream = MakeHolder<TFileInput>(options.Path);
        } else {
            resourceDataHolder = NResource::Find(options.Resource);
            configInputStream = MakeHolder<TStringInput>(resourceDataHolder);
        }
        if (options.ConfigFormat == TLoadConfigOptions::EConfigFormat::Json) {
            ParseConfigFromJson(*configInputStream, config, options.Json2ProtoOptions);
        } else if (options.ConfigFormat == TLoadConfigOptions::EConfigFormat::ProtoText) {
            ParseFromTextFormat(*configInputStream, config);
        } else {
            ythrow NLastGetopt::TUsageException() << "unknown format: " << options.ConfigFormat;
        }

        for (const TString& ov : options.Overrides) {
            OverrideConfig(config, ov);
        }
    }
}
