#include "server.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/buffer.h>

namespace {
    class TTestCallback: public NUtil::TTcpServer::ICallback {
    public:
        virtual void OnConnection(NUtil::TTcpServer::TConnectionPtr connection) override {
            CHECK_WITH_LOG(connection);
            CHECK_WITH_LOG(connection->Alive());
            INFO_LOG << connection->GetRemoteAddr() << Endl;
            connection->Read([connection](const TBuffer& buffer) {
                INFO_LOG << TStringBuf(buffer.Data(), buffer.End()) << Endl;
                TBuffer echo(buffer);
                connection->Write(std::move(echo));
                connection->Drop();
            });
        }
    };
}

Y_UNIT_TEST_SUITE(TCPServerSuite) {
    Y_UNIT_TEST(Simple) {
        InitGlobalLog2Console(TLOG_DEBUG);
        TPortManager ports;
        const auto port = ports.GetPort(10000);

        auto callback = MakeAtomicShared<TTestCallback>();
        NUtil::TTcpServer::TOptions options;
        NUtil::TTcpServer server(port, options, callback);

        server.Start();

        TNetworkAddress na("localhost", port);
        TSocket socket(na);
        TSocketInput input(socket);
        TSocketOutput output(socket);

        TString request = "Hello, world!";
        output << request;
        output.Flush();
        TString response = input.ReadAll();

        UNIT_ASSERT_VALUES_EQUAL(request, response);
        server.Stop();
    }
}
