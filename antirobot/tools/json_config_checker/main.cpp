#include <antirobot/daemon_lib/config.h>
#include <antirobot/daemon_lib/prepared_rule.h>
#include <antirobot/daemon_lib/yql_rule_set.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/iterator/concatenate.h>
#include <library/cpp/iterator/enumerate.h>

#include <util/string/builder.h>

using namespace NAntiRobot;


struct TArgs {
    TString ServiceConfig;
    TString ServiceIdentifier;
    TString GlobalConfig;

    static TArgs Parse(int argc, char* argv[]) {
        NLastGetopt::TOpts opts;
        TArgs args;

        opts.SetTitle("check antirobot config files");

        opts.AddLongOption("service-config", "service_config.json")
            .RequiredArgument("SERVICE_CONFIG_PATH")
            .StoreResult(&args.ServiceConfig);

        opts.AddLongOption("service-identifier", "service_identifier.json")
            .RequiredArgument("SERVICE_IDENTIFIER_PATH")
            .StoreResult(&args.ServiceIdentifier);

        opts.AddLongOption("global-config", "global_config.json")
            .RequiredArgument("GLOBAL_CONFIG_PATH")
            .StoreResult(&args.GlobalConfig);

        opts.SetFreeArgsMax(0);
        opts.AddHelpOption();

        NLastGetopt::TOptsParseResult parseResult(&opts, argc, argv);
        Y_UNUSED(parseResult);

        Y_ENSURE(
            args.ServiceConfig.empty() == args.ServiceIdentifier.empty(),
            "SERVICE_CONFIG_PATH and SERVICE_IDENTIFIER_PATH must either be "
            "both set or both unset"
        );

        return args;
    }
};


int main(int argc, char* argv[]) {
    const auto args = TArgs::Parse(argc, argv);

    bool ok = true;

    if (!args.ServiceConfig.empty()) {
        TAntirobotDaemonConfig cfg;

        try {
            cfg.JsonConfig.LoadFromFile(args.ServiceConfig, args.ServiceIdentifier);
        } catch (const yexception& exc) {
            Cerr << "Invalid service config / service identifier pair: " << exc.what();
            ok = false;
        }
    }

    if (!args.GlobalConfig.empty()) {
        TGlobalJsonConfig cfg;
        cfg.LoadFromFile(args.GlobalConfig);

        TStringBuilder whatRule;
        TVector<TString> cbbErrors;

        for (const auto [i, rule] : Enumerate(Concatenate(cfg.Rules, cfg.MarkRules))) {
            whatRule.clear();
            cbbErrors.clear();

            if (i < cfg.Rules.size()) {
                whatRule << "rule " << i;
            } else {
                whatRule << "mark rule " << (i - cfg.Rules.size());
            }

            const auto parsedCbbRules = TPreparedRule::ParseList(rule.Cbb, &cbbErrors);
            Y_UNUSED(parsedCbbRules);

            if (!cbbErrors.empty()) {
                ok = false;
            }

            for (const auto& cbbError : cbbErrors) {
                Cerr << "Invalid CBB in " << whatRule << ": " << cbbError << '\n';
            }

            try {
                TYqlRuleSet yqlRules(rule.Yql);
            } catch (const yexception& exc) {
                Cerr << "Invalid YQL in " << whatRule << ": " << exc.what() << '\n';
                ok = false;
            }
        }

        for (const auto [i, rule] : Enumerate(cfg.LastVisitsRules)) {
            try {
                TPreparedRule::Parse(rule.Rule);
            } catch (const std::exception& exc) {
                Cerr
                    << "Invalid last visits rule "
                    << i << " (id=" << rule.Id << ", name=" << rule.Name << "): "
                    << exc.what() << '\n';
            }
        }
    }

    return ok ? 0 : 1;
}
