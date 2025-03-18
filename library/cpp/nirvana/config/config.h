#pragma once

#include <library/cpp/nirvana/job_context/job_context.h>

#include <library/cpp/config/config.h>
#include <library/cpp/config/domscheme.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/ysaveload.h>

namespace NNirvana {
    enum class EConfigSchemaValidation {
        Default,
        Strict,
    };
    template <template <class> class T>
    class TConfigOption {
    public:
        using TConfig = T<TConfigTraits>;

        TConfigOption(NLastGetopt::TOpts& parser, EConfigSchemaValidation validation = EConfigSchemaValidation::Default);

        const TConfig& Config() {
            if (!Loaded) {
                Load();
                Loaded = true;
            }

            return *ParsedConfig;
        }

    private:
        void Load();

        TFsPath ConfigFileName;
        TFsPath VarsFileName;
        bool DumpConfig = false;
        THashMap<TString, TString> Params;
        EConfigSchemaValidation Validation;

        NConfig::TConfig ConfigRaw;
        TMaybe<TConfig> ParsedConfig;
        bool Nirvana = false;

        bool Loaded = false;
    };

    NConfig::TConfig LoadFromFile(const TFsPath& filename, const NConfig::TGlobals& globals);

    template <template <class> class T>
    TConfigOption<T>::TConfigOption(NLastGetopt::TOpts& parser, EConfigSchemaValidation validation /*= EConfigSchemaValidation::Default*/)
        : Validation(validation)
    {
        parser
            .AddLongOption('c', "config", "-- config file (.json, .lua, .ini extensions are supported)")
            .Optional()
            .RequiredArgument("FILE")
            .StoreResult(&ConfigFileName);

        parser
            .AddLongOption("vars", "-- json file with global variable overrides for config")
            .Optional()
            .RequiredArgument("FILE")
            .StoreResult(&VarsFileName);

        parser
            .AddLongOption("param", "-- pass parameters to config preprocessor")
            .RequiredArgument("KEY=VALUE")
            .KVHandler([this](TString key, TString value) {
                Params[key] = value;
            });

        parser
            .AddLongOption("dump-config", "-- dump resulting config after loading")
            .Optional()
            .NoArgument()
            .SetFlag(&DumpConfig);
    }

    template <template <class> class T>
    void TConfigOption<T>::Load() {
        NConfig::TGlobals globals;

        if (VarsFileName) {
            NJson::TJsonValue vars;
            TFileInput file(VarsFileName);
            NJson::ReadJsonTree(&file, &vars, true);
            Y_ENSURE(vars.IsMap(), "Vars must contain a json map");
            for (const auto& pair : vars.GetMap()) {
                globals[pair.first] = pair.second;
            }
        }

        const auto jobContextHolder = LoadJobContext<TMrJobContextData>();
        Nirvana = !!jobContextHolder;
        if (Nirvana) {
            const auto context = jobContextHolder->Context();

            globals["nirvana"] = jobContextHolder->RawContext();

            if (!ConfigFileName) {
                const auto config = context.Inputs()["config"];
                if (!config.IsNull()) {
                    ConfigFileName = TFsPath(config[0]);
                }
            }
        }

        // TODO: introduce compiled-in config and try to use it right here
        Y_ENSURE(ConfigFileName, "No config available, either pass --config option or run in nirvana with input named `config'");

        for (const auto& pair : Params) {
            globals[pair.first] = pair.second;
        }

        ConfigRaw = LoadFromFile(ConfigFileName, globals);

        ParsedConfig.ConstructInPlace(&ConfigRaw);

        TStringBuilder diag;
        if (!ParsedConfig->Validate({}, Validation == EConfigSchemaValidation::Strict, [&diag](TString lhs, TString rhs) {
                diag << lhs << ": " << rhs << Endl;
            })) {
            ythrow yexception() << "Config schema validation failed:\n"
                                << diag;
        }

        if (DumpConfig) {
            ConfigRaw.DumpJson(Cerr);
        }

        if (Nirvana) {
            TFixedBufferFileOutput file("config.log");
            ConfigRaw.DumpJson(file);
        }
    }

    template <template <class> class T>
    class TSchematizedConfig {
    public:
        using TSchema = T<TConfigTraits>;
        TSchematizedConfig()
            : Schema(&RawConfig)
        {
        }

        TSchematizedConfig(const NConfig::TConfig& config)
            : RawConfig(config)
            , Schema(&RawConfig)
        {
        }

        TSchematizedConfig(NConfig::TConfig&& config)
            : RawConfig(std::move(config))
            , Schema(&RawConfig)
        {
        }

        TSchematizedConfig(const TSchematizedConfig<T>& rhs)
            : TSchematizedConfig(rhs.RawConfig)
        {
        }

        TSchematizedConfig(TSchematizedConfig<T>&& rhs)
            : TSchematizedConfig(std::move(rhs.RawConfig))
        {
        }

        TSchematizedConfig(const TSchema& schema)
            : TSchematizedConfig(schema.GetRawValue())
        {
        }

        TSchematizedConfig(TSchema&& schema)
            : TSchematizedConfig(std::move(*schema.GetRawValue()))
        {
        }

        template <template <class> class U>
        operator TSchematizedConfig<U>() {
            return TSchematizedConfig<U>(RawConfig);
        }

        const TSchema& Config() const {
            return Schema;
        }

        const TSchema* operator->() const {
            return &Schema;
        }

        const TSchema& operator*() const {
            return Schema;
        }

        Y_SAVELOAD_DEFINE(RawConfig);

    private:
        NConfig::TConfig RawConfig; // Order of fields is important!
        TSchema Schema;
    };
}
