#include <infra/yp_service_discovery/libs/sdlib/server_mock/server.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/hash_set.h>
#include <util/stream/output.h>
#include <util/system/mutex.h>


struct TConfig {
    ui16 AdminPort;
    ui16 DiscoveryPort;

    static TConfig ParseArgs(int argc, char* argv[]) {
        NLastGetopt::TOpts opts;
        TConfig config;

        opts.SetTitle("mock YP discovery");

        opts.AddLongOption("admin-port", "admin port")
            .RequiredArgument("ADMIN_PORT")
            .DefaultValue(9090)
            .StoreResult(&config.AdminPort);

        opts.AddLongOption("discovery-port", "discovery port")
            .RequiredArgument("DISCOVERY_PORT")
            .DefaultValue(8080)
            .StoreResult(&config.DiscoveryPort);

        opts.SetFreeArgsMax(0);
        opts.AddHelpOption();

        NLastGetopt::TOptsParseResult result(&opts, argc, argv);
        Y_UNUSED(result);

        return config;
    }
};


struct TState {
    NYP::NServiceDiscovery::NTesting::TSDServer DiscoveryServer;

    TMutex Mutex;
    TVector<NYP::NServiceDiscovery::NApi::TReqResolveEndpoints> Reqs;

    explicit TState(const NYP::NServiceDiscovery::NTesting::TSDServer::TOptions& options) :
        DiscoveryServer(options)
    {}
};


class TReplier : public TRequestReplier {
public:
    explicit TReplier(TState* state)
        : State(state)
    {}

    bool DoReply(const TReplyParams& params) override {
        TParsedHttpFull req(params.Input.FirstLine());

        try {
            Y_ENSURE(req.Method == "POST", "expected a POST method");

            NJson::TJsonValue input;
            NJson::ReadJsonTree(&params.Input, &input, true);

            with_lock (State->Mutex) {
                if (req.Path == "/set") {
                    params.Output << HandleSet(input);
                } else if (req.Path == "/remove") {
                    params.Output << HandleRemove(input);
                } else if (req.Path == "/clear") {
                    params.Output << HandleClear(input);
                }
            }
        } catch (...) {
            const auto msg = CurrentExceptionMessage();
            Cerr << "discovery_mock: error: " << msg << Endl;

            NJsonWriter::TBuf errorResponse;
            errorResponse
                .BeginObject()
                    .WriteKey("error").WriteString(msg)
                .EndObject();

            params.Output << THttpResponse(HTTP_BAD_REQUEST)
                .SetContent(errorResponse.Str(), "application/json");
        }

        return true;
    }

private:
    THttpResponse HandleSet(const NJson::TJsonValue& input) const {
        const auto req = ParseProto<NYP::NServiceDiscovery::NApi::TReqResolveEndpoints>(input, "req");
        const auto rsp = ParseProto<NYP::NServiceDiscovery::NApi::TRspResolveEndpoints>(input, "rsp");
        State->DiscoveryServer.Set(req, rsp);

        State->Reqs.push_back(req);

        return THttpResponse(HTTP_OK);
    }

    THttpResponse HandleRemove(const NJson::TJsonValue& input) const {
        const auto req = ParseProto<NYP::NServiceDiscovery::NApi::TReqResolveEndpoints>(input, "req");
        State->DiscoveryServer.Remove(req);

        return THttpResponse(HTTP_OK);
    }

    THttpResponse HandleClear(const NJson::TJsonValue& input) const {
        Y_UNUSED(input);

        for (const auto& req : State->Reqs) {
            State->DiscoveryServer.Remove(req);
        }

        State->Reqs.clear();

        return THttpResponse(HTTP_OK);
    }

    template <typename T>
    static T ParseProto(const NJson::TJsonValue& input, TStringBuf key) {
        T x;

        const auto bytes = Base64StrictDecode(input[key].GetStringSafe());
        Y_ENSURE(x.ParseFromString(bytes), "Failed to parse '" << key << "'");

        return x;
    }

private:
    TState* State;
};


class TCallback : public THttpServer::ICallBack {
public:
    explicit TCallback(TState* state)
        : State(state)
    {}

    TClientRequest* CreateClient() override {
        return new TReplier(State);
    }

private:
    TState* State;
};


int main(int argc, char* argv[]) {
    const auto config = TConfig::ParseArgs(argc, argv);

    const auto discoveryServerOptions = NYP::NServiceDiscovery::NTesting::TSDServer::TOptions()
        .SetPort(config.DiscoveryPort);

    TState state(discoveryServerOptions);
    state.DiscoveryServer.Start();

    const auto adminServerOptions = THttpServerOptions(config.AdminPort)
        .SetThreads(1);

    TCallback adminCallback(&state);
    THttpServer adminServer(&adminCallback, adminServerOptions);

    if (!adminServer.Start()) {
        Cerr << "Failed to start admin server: " << adminServer.GetError() << Endl;
        return 1;
    }

    adminServer.Wait();
}
