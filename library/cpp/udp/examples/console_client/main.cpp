#include <library/cpp/udp/client/client.h>

int main() {
    Cout << "Server host: ";
    TString host;
    Cin >> host;
    Cout << "Server port: ";
    ui32 port;
    Cin >> port;
    Cout << "Message: ";
    TString message;
    Cin >> message;
    NUdp::SendUdpPacket(TNetworkAddress(host, port), message);
}
