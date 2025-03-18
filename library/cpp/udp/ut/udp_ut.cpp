#include <library/cpp/udp/client/client.h>
#include <library/cpp/udp/server/server.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>

Y_UNIT_TEST_SUITE(TUdpTest) {

    Y_UNIT_TEST(TClientServerTest) {
        TPortManager portManager;
        ui16 port = portManager.GetTcpAndUdpPort();

        NUdp::TServerOptions options;
        options.SetPort(port)
            .SetWorkers(IsReusePortAvailable() ? 10 : 1)
            .SetExecutors(1);

        THashSet<TString> recievedPackets;
        NUdp::TUdpServer server(options, [&recievedPackets](NUdp::TUdpPacket packet) {
            recievedPackets.emplace(packet.Data);
        });
        server.Start();

        for (size_t index = 0; index < 1000; ++index) {
            NUdp::SendUdpPacket(TNetworkAddress("localhost", port), ToString(index));
        }

        Sleep(TDuration::Seconds(1));

        size_t lostPackets = 0;
        for (size_t index = 0; index < 1000; ++index) {
            if (!recievedPackets.count(ToString(index))) {
                lostPackets++;
            }
        }
        UNIT_ASSERT(lostPackets <= 10);
    }

    Y_UNIT_TEST(TTestNoExecutors) {
        TPortManager portManager;
        ui16 port = portManager.GetTcpAndUdpPort();

        NUdp::TServerOptions options;
        options.SetPort(port)
            .SetWorkers(IsReusePortAvailable() ? 3 : 1)
            .SetExecutors(0);

        THashSet<TString> recievedPackets;
        NUdp::TUdpServer server(options, [&recievedPackets](NUdp::TUdpPacket packet) {
            recievedPackets.emplace(packet.Data);
        });
        server.Start();

        for (size_t index = 0; index < 1000; ++index) {
            NUdp::SendUdpPacket(TNetworkAddress("localhost", port), ToString(index));
        }

        Sleep(TDuration::Seconds(1));

        size_t lostPackets = 0;
        for (size_t index = 0; index < 1000; ++index) {
            if (!recievedPackets.count(ToString(index))) {
                lostPackets++;
            }
        }
        UNIT_ASSERT(lostPackets <= 500);
    }

    Y_UNIT_TEST(TTestLargePackets) {
        TPortManager portManager;
        ui16 port = portManager.GetTcpAndUdpPort();

        NUdp::TServerOptions options;
        options.SetPort(port);
        options.SetWorkers(IsReusePortAvailable() ? 5 : 1);

        THashSet<TString> recievedPackets;
        NUdp::TUdpServer server(options, [&recievedPackets](NUdp::TUdpPacket packet) {
            recievedPackets.emplace(packet.Data);
        });
        server.Start();

        TString cowboy(100000, 'a');
        UNIT_ASSERT_EXCEPTION(NUdp::SendUdpPacket(TNetworkAddress("localhost", port), cowboy), yexception);

        cowboy.resize(50000);
        NUdp::SendUdpPacket(TNetworkAddress("localhost", port), cowboy);

        Sleep(TDuration::Seconds(1));
        UNIT_ASSERT(recievedPackets.size() == 1 && *recievedPackets.begin() == TString(50000, 'a'));
    }

    Y_UNIT_TEST(TTestFailedBind) {
        NUdp::TServerOptions options;
        options.SetPort(80);
        options.SetWorkers(IsReusePortAvailable() ? 5 : 1);

        NUdp::TUdpServer server(options, [](NUdp::TUdpPacket) {});
        UNIT_ASSERT_EXCEPTION(server.Start(), yexception);
    }

    Y_UNIT_TEST(TTestRestart) {
        TPortManager portManager;
        ui16 port = portManager.GetTcpAndUdpPort();

        NUdp::TServerOptions options;
        options.SetPort(port);
        options.SetWorkers(IsReusePortAvailable() ? 5 : 1);

        THashSet<TString> recievedPackets;
        NUdp::TUdpServer server(options, [&recievedPackets](NUdp::TUdpPacket packet) {
            recievedPackets.emplace(packet.Data);
        });
        server.Start();

        NUdp::SendUdpPacket(TNetworkAddress("localhost", port), TString(10, 'a'));

        server.Stop();
        Sleep(TDuration::Seconds(1));
        NUdp::SendUdpPacket(TNetworkAddress("localhost", port), TString(10, 'b'));

        server.Start();
        NUdp::SendUdpPacket(TNetworkAddress("localhost", port), TString(10, 'c'));
        Sleep(TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(recievedPackets.size(), 2);
        UNIT_ASSERT(recievedPackets.count(TString(10, 'a')) && recievedPackets.count(TString(10, 'c')));
    }
}
