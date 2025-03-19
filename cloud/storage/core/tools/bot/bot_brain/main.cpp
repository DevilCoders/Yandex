#include "auto_teamlead.h"
#include "neocortex_bot.h"

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/neh/rpc.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/printf.h>
#include <util/system/env.h>

#include <cmath>

using namespace NCoolBot;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    ui16 Port = 0;
    TVector<TString> BotSpecs;
    TString AutoTeamleadSpec;
};

////////////////////////////////////////////////////////////////////////////////

struct TServer
{
    THolder<TAutoTeamlead> AutoTeamlead;
    THashMap<TString, TBotPtr> Bots;

    void ServeRequest(const NNeh::IRequestRef& req)
    {
        try {
            NJson::TJsonValue v;
            NJson::ReadJsonTree(req->Data(), &v, true);
            const auto& name = v["name"];
            if (name.IsString()) {
                if (auto bot = Bots.FindPtr(name.GetStringSafe())) {
                    const auto& login = v["login"];
                    const auto& chatId = v["chat_id"];
                    const auto& context = v["context"];
                    if (context.IsString()) {
                        NJson::TJsonValue result;
                        TNeocortexBot::TResult answer;
                        TString sticker;
                        bool selected = AutoTeamlead->SelectAnswer(
                            login.GetString(),
                            context.GetString(),
                            chatId.GetString(),
                            &answer.Text,
                            &sticker
                        );

                        if (!selected) {
                            answer = (*bot)->SelectAnswer(context.GetString());
                        }

                        if (answer.Text || sticker) {
                            if (answer.Text) {
                                result["answer"] = std::move(answer.Text);
                            }
                            if (sticker) {
                                result["sticker"] = std::move(sticker);
                            }
                            result["score"] = answer.Score;
                        }

                        NNeh::TDataSaver out;
                        NJson::WriteJson(&out, &result);
                        out << Endl;

                        req->SendReply(out);
                    } else {
                        req->SendError(
                            NNeh::IRequest::BadRequest,
                            "context not specified"
                        );
                    }
                } else {
                    req->SendError(
                        NNeh::IRequest::BadRequest,
                        Sprintf("no such bot: %s", name.GetStringSafe().c_str())
                    );
                }
            } else {
                req->SendError(
                    NNeh::IRequest::BadRequest,
                    "bot name not specified"
                );
            }
        } catch (...) {
            req->SendError(
                NNeh::IRequest::ServiceUnavailable,
                CurrentExceptionMessage()
            );
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char* argv[]) {
    TOptions options;

    {
        using namespace NLastGetopt;

        TOpts opts = TOpts::Default();

        opts.AddHelpOption();

        opts.AddCharOption('p')
            .AddLongName("port")
            .StoreResult(&options.Port)
            .RequiredArgument("PORT")
            .DefaultValue("11111")
            .Help("api port");

        opts.AddCharOption('b')
            .AddLongName("bot-spec")
            .AppendTo(&options.BotSpecs)
            .RequiredArgument("PATH")
            .Help("path to bot config");

        opts.AddCharOption('a')
            .AddLongName("auto-teamlead-spec")
            .StoreResult(&options.AutoTeamleadSpec)
            .RequiredArgument("PATH")
            .Help("path to autoteamlead config");

        TOptsParseResult(&opts, argc, argv);
    }

    try {
        TServer server;

        for (const auto& spec: options.BotSpecs) {
            NJson::TJsonValue config;
            NJson::ReadJsonTree(TIFStream(spec).ReadAll(), &config, true);
            const auto& botName = config["name"].GetStringSafe();
            server.Bots[botName] = new TNeocortexBot(config);
        }

        {
            NJson::TJsonValue config;
            if (options.AutoTeamleadSpec) {
                NJson::ReadJsonTree(
                    TIFStream(options.AutoTeamleadSpec).ReadAll(),
                    &config,
                    true
                );
            }

            server.AutoTeamlead.Reset(new TAutoTeamlead(
                GetEnv("ST_TOKEN"),
                config
            ));
        }

        NNeh::IServicesRef sref = NNeh::CreateLoop();
        sref->Add(Sprintf("post://*:%u/select_answer", options.Port), server);
        sref->Loop(1);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;

        return 1;
    }

    return 0;
}
