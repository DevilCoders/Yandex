#include <library/cpp/http/coro/server.h>
#include <library/cpp/http/server/response.h>

using namespace NCoroHttp;

int main() {
    THttpServer srv;

    srv.RunCycle(THttpServer::TCallbacks().SetRequestCb([&srv](THttpServer::TRequestContext& ctx) {
        *ctx.Output << THttpResponse().SetContent("Hellow, World!");
        srv.ShutDown();
    }));
}
