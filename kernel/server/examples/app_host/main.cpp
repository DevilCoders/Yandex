#include <apphost/lib/common/exceptions.h>
#include <apphost/api/service/cpp/service.h>

#include <kernel/server/apphost_request.h>
#include <kernel/server/protos/serverconf.pb.h>
#include <kernel/server/server.h>

#include <util/generic/ptr.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/system/info.h>

class TRequest final : public ::NServer::IApphostRequest {
public:
    TRequest() = default;
    ~TRequest() override = default;

    void Reply(NAppHost::IServiceContext& ctx, void*) const override {
        if (ctx.GetItemRefs("do_404")) {
            ythrow ::NAppHost::TBadRequestException{} << "404 as requested";
        }

        ctx.AddItem("Hello, World!", "greeting");

        ctx.AddLogLine("I'm a log message");
        ctx.AddLogLine("I'm a second log message");
    }
};

class TServer final : public ::NServer::TServer {
public:
    explicit TServer(const ::NServer::THttpServerConfig& config)
        : ::NServer::TServer{config} {
    }
    ~TServer() override = default;

private:
    THolder<::NServer::IApphostRequest> CreateApphostClient() override {
        return MakeHolder<TRequest>();
    }
};

int main() {
    // use default config (you can find default values in `serverconf.proto`)
    NServer::THttpServerConfig config;
    config.SetEnableAppHost(true);
    config.SetAppHostThreads(Max<ui32>(1, NSystemInfo::CachedNumberOfCpus() - 1));

    TServer server{config};
    server.Start();
    server.Wait();

    // Now you can send requests to it:
    //
    // # yazevnul @ mt-dev-01 in ~/arc/apphost/tools/servant_client [11:45:46]
    // $ ./servant_client --pretty 'localhost:17001' --context '{}'
    // time    1139
    // size    323
    // {
    //     "answers":
    //         [
    //             {
    //                 "data":"Hello, World!",
    //                 "type":"greeting"
    //             }
    //         ],
    //     "extra_log":
    //         [
    //             "I'm a log message",
    //             "I'm a second log message"
    //         ]
    // }
    //
    // # yazevnul @ mt-dev-01 in ~/arc/apphost/tools/servant_client [11:47:33]
    // $ ./servant_client --pretty 'localhost:17001' --context '[{"meta": {}, "name": "INIT", "results": [{"type":"do_404"}]}]'
    // time    1336
    // size    251
    // {
    //     "error":"(NAppHost::TBadRequestException) kernel/server/examples/app_host/main.cpp:20: 404 as requested"
    // }
    //
    // # yazevnul @ mt-dev-01 in ~/arc/apphost/tools/servant_client [11:47:36]
    // $
    //

    return 0;
}
