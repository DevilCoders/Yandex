#include <library/cpp/xmlrpc/client/client.h>
#include <library/cpp/xmlrpc/server/server.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/string/subst.h>

Y_UNIT_TEST_SUITE(TXmlRPCTest) {
    using namespace NNeh;
    using namespace NXmlRPC;

    static int F(int i, int j) {
        return i + j;
    }

    static int E(int i, int j) {
        return i - j;
    }

    static ui16 AddLoop(IServicesRef & loop, const TString& addr, IServiceRef srv, TPortManager& pm) {
        try {
            ui16 portToBind = pm.GetPort();
            TString realAddr(addr);

            SubstGlobal(realAddr, "*:*", "*:" + ToString(portToBind));
            loop->Add(realAddr, srv);

            return portToBind;
        } catch (...) {
            //Cerr << CurrentExceptionMessage() << Endl;
        }

        ythrow yexception() << "can not bind on random port for " << addr;
    }

    Y_UNIT_TEST(TestHttp) {
        TPortManager portManager;
        IServerRef srv = CreateServer();
        IServicesRef loop = CreateLoop();
        const ui16 port = AddLoop(loop, "http://*:*/xmlrpc", IServiceRef(&srv->Add("sum", F).Add("sub", E)), portManager);

        loop->ForkLoop(1);

        TEndPoint ep("http://localhost:" + ToString(port) + "/xmlrpc");

        UNIT_ASSERT_EQUAL(Cast<int>(ep.AsyncCall("sum", 1, 2)->Wait()), 3);
        UNIT_ASSERT_EQUAL(Cast<int>(ep.AsyncCall("sub", 1, "2")->Wait()), -1);
    }

    Y_UNIT_TEST(TestNetliba) {
        TPortManager portManager;
        IServerRef srv = CreateServer();
        IServicesRef loop = CreateLoop();
        const ui16 port = AddLoop(loop, "netliba://*:*/xmlrpc", IServiceRef(&srv->Add("sum", F).Add("sub", E)), portManager);

        loop->ForkLoop(1);

        TEndPoint ep("netliba://localhost:" + ToString(port) + "/xmlrpc");

        UNIT_ASSERT_EQUAL(Cast<int>(ep.AsyncCall("sum", 1, 2)->Wait()), 3);
        UNIT_ASSERT_EQUAL(Cast<int>(ep.AsyncCall("sub", 1, "2")->Wait()), -1);
    }
}
