#include "old_match_rules.h"

#include <antirobot/daemon_lib/environment.h>
#include <antirobot/daemon_lib/rule_set.h>
#include <antirobot/daemon_lib/fullreq_info.h>
#include <antirobot/daemon_lib/match_rule_parser.h>

#include <contrib/libs/benchmark/include/benchmark/benchmark.h>

#include <library/cpp/iterator/concatenate.h>
#include <library/cpp/iterator/enumerate.h>

#include <library/cpp/json/json_reader.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <library/cpp/testing/common/env.h>

#include <util/folder/dirut.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/string/strip.h>
#include <util/stream/file.h>
#include <util/system/shellcommand.h>
#include <util/system/tempfile.h>

using namespace NAntiRobot;


struct TBenchData {
    THolder<TEnv> Env;
    TVector<TFullReqInfo> Requests;

    static const TBenchData& Instance() {
        static const TBenchData benchData;
        return benchData;
    }

    void WaitForCbb() const {
        // TODO(rzhikharevich): replace this workaround with something more reliable.
        do {
            sleep(1);
        } while (Env->CbbIO->QueueSize() > 0);
    }

private:
    TBenchData() {
        Env = LoadEnv();
        Requests = LoadRequests(*Env);
    }

    static THolder<TEnv> LoadEnv() {
        MakeDirIfNotExist("antirobot/runtime");

        TStringBuilder gencfgOutput;

        TShellCommand gencfgCmd(
            BinaryPath("antirobot/scripts/gencfg/antirobot_gencfg"),
            {"--local"},
            TShellCommandOptions()
                .SetUseShell(false)
                .SetOutputStream(&gencfgOutput.Out)
                .SetErrorStream(&Cerr)
        );
        gencfgCmd.Run();

        TString configPath = "antirobot.cfg";
        TFileOutput configOutput(configPath);

        const THashMap<TString, TString> configKeyToValue = {
            {"BaseDir", "antirobot"},
            {"JsonConfFilePath", ArcadiaSourceRoot() + "/antirobot/config/service_config.json"},
            {"GlobalJsonConfFilePath", ArcadiaSourceRoot() + "/antirobot/daemon_lib/benchmarks/rule_set/global_config.json"},
            {"JsonServiceRegExpFilePath", ArcadiaSourceRoot() + "/antirobot/config/service_identifier.json"},
            {"ExperimentsConfigFilePath", ""},
            {"AllDaemons", "localhost:80"},
            {"RuntimeDataDir", "runtime"},
            {"UnifiedAgentUri", ""},
            {"LogsDir", "runtime"}
        };

        for (const auto& it : StringSplitter(gencfgOutput).Split('\n')) {
            const size_t keyStart = it.Token().find_first_not_of(' ');

            if (keyStart == TStringBuf::npos) {
                configOutput << it.Token() << '\n';
                continue;
            }

            TStringBuf key;
            TStringBuf value;

            if (!it.Token().Skip(keyStart).TrySplit(" =", key, value)) {
                configOutput << it.Token() << '\n';
                continue;
            }

            if (const auto newValue = configKeyToValue.FindPtr(key)) {
                configOutput << it.Token().Head(keyStart) << key << " = " << *newValue << '\n';
            } else {
                configOutput << it.Token() << '\n';
            }
        }

        configOutput.Flush();

        ANTIROBOT_DAEMON_CONFIG_MUTABLE.SetBaseDir("antirobot");
        ANTIROBOT_CONFIG_MUTABLE.LoadFromPath(configPath);
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.UseTVMClient = false;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.CaptchaApiHost = THostAddr();
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.CbbApiHost = THostAddr(TNetworkAddress("cbb-testing.n.yandex-team.ru", 80), "cbb-testing.n.yandex-team.ru", 80, "cbb-testing.n.yandex-team.ru");
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.DiscoveryHost = "";
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.ThreadPoolParams = "free_min=0; free_max=0; total_max=0; increase=1";

        return MakeHolder<TEnv>();
    }

    static TVector<TFullReqInfo> LoadRequests(const TEnv& env) {
        TVector<TFullReqInfo> requests;

        TVector<TString> fileNames;
        TFsPath(".").ListNames(fileNames);

        TString requestFileName;

        for (const auto& fileName : fileNames) {
            if (fileName.StartsWith("reqdata_")) {
                requestFileName = fileName;
            }
        }

        Y_ENSURE(!requestFileName.empty(), "failed to find request file");

        TFileInput input(requestFileName);
        TString line;
        TString data;
        const TString requesterAddr = "127.0.0.1";

        while (input.ReadLine(line) > 0) {
            Base64StrictDecode(line, data);

            TStringInput dataInput(data);
            THttpInput httpInput(&dataInput);

            TFullReqInfo request(
                httpInput,
                data,
                requesterAddr,
                env.ReloadableData,
                env.PanicFlags,
                GetSpravkaIgnorePredicate(env),
                &env.ReqGroupClassifier,
                &env
            );

            requests.push_back(std::move(request));
        }

        return requests;
    }
};


struct TCbbData {
    TVector<std::pair<TCbbGroupId, TVector<TPreparedRule>>> Rules;
    TVector<std::pair<TCbbGroupId, TVector<TRegexMatcherEntry>>> PatternLists;

    static const TCbbData& Instance() {
        static const TCbbData cbbData;
        return cbbData;
    }

    size_t CountRules() const {
        size_t sum = 0;

        for (const auto& [_, rules] : Rules) {
            sum += rules.size();
        }

        return sum;
    }

private:
    TCbbData() {
        TFileInput jsonFile("cbb.json");

        NJson::TJsonValue json;
        NJson::ReadJsonTree(&jsonFile, &json, true);

        InitRules(json);
        InitPatternLists(json);
    }

    void InitRules(const NJson::TJsonValue& json) {
        for (const auto& [groupId, rules] : json["txt"].GetMapSafe()) {
            ui32 iGroupId = FromString<ui32>(groupId);
            if (iGroupId != 262 && iGroupId != 183) {
                continue;
            }

            const auto& rulesVec = rules.GetArraySafe();

            TVector<TPreparedRule> parsedRules;
            parsedRules.reserve(rulesVec.size());

            for (const auto& rule : rulesVec) {
                parsedRules.push_back(TPreparedRule::Parse(rule.GetStringSafe()));
            }

            Rules.push_back({FromString<TCbbGroupId>(groupId), parsedRules});
        }
    }

    void InitPatternLists(const NJson::TJsonValue& json) {
        for (const auto& [groupId, patterns] : json["re"].GetMapSafe()) {
            const auto& patternsVec = patterns.GetArraySafe();

            TVector<TRegexMatcherEntry> parsedPatterns;
            parsedPatterns.reserve(patternsVec.size());

            for (const auto& pattern : patternsVec) {
                const auto parsedPattern = TRegexMatcherEntry::Parse(pattern.GetStringSafe());
                Y_ENSURE(parsedPattern.Defined(), "failed to parse: " << pattern.GetStringSafe());
                parsedPatterns.push_back(*parsedPattern);
            }

            PatternLists.push_back({FromString<TCbbGroupId>(groupId), std::move(parsedPatterns)});
        }
    }
};


void BenchAllOld(benchmark::State& state) {
    const auto& data = TBenchData::Instance();
    data.WaitForCbb();

    const auto& cbbData = TCbbData::Instance();

    TVector<NOldMatchRequest::TPreparedMatchRule> preparedRules;
    preparedRules.reserve(cbbData.Rules.size());

    for (const auto& [_, rules] : cbbData.Rules) {
        for (const auto& rule : rules) {
            preparedRules.push_back(NOldMatchRequest::TPreparedMatchRule(
                rule,
                &data.Env->NonBlockingAddrs,
                &data.Env->Alarmer,
                data.Env->CbbIO.Get(),
                &data.Env->CbbIpListManager,
                &data.Env->CbbReListManager
            ));
        }
    }

    data.WaitForCbb();

    const auto beforeRuleSet = TInstant::Now();
    NOldMatchRequest::TRuleSet ruleSet(std::move(preparedRules));
    Cerr << "OLD RULESET: " << (TInstant::Now() - beforeRuleSet) << Endl;

    for (auto _ : state) {
        for (const auto& req : data.Requests) {
            auto result = ruleSet.Match(req);
            const TVector<TCbbRuleId> resultVec(result.begin(), result.end());
            benchmark::DoNotOptimize(resultVec);
        }
    }
}

BENCHMARK(BenchAllOld);


void BenchAllNew(benchmark::State& state) {
    const auto& data = TBenchData::Instance();
    data.WaitForCbb();

    const auto& cbbData = TCbbData::Instance();

    const auto beforeRuleSet = TInstant::Now();
    TRuleSet ruleSet(
        cbbData.Rules,
        data.Env->NonBlockingAddrs,
        &data.Env->CbbIpListManager,
        &data.Env->CbbReListManager
    );
    Cerr << "RULESET: " << (TInstant::Now() - beforeRuleSet) << Endl;
    Cerr << "NUM ROOTS: " << ruleSet.NumRoots() << Endl;

    data.WaitForCbb();

    for (auto _ : state) {
        for (const auto& req : data.Requests) {
            benchmark::DoNotOptimize(ruleSet.Match(req));
        }
    }
}

BENCHMARK(BenchAllNew);
