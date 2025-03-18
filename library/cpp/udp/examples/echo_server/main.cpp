#include <library/cpp/udp/server/server.h>

using namespace NUdp;

int main() {
    TServerOptions options;
    options
        .SetPort(1234)
        .SetWorkers(10);
    TUdpServer server(options, [](TUdpPacket packet) { Cerr << packet.Data << Endl; });
    server.Start();
    server.Wait();
}
