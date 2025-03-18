#include <library/cpp/testing/unittest/registar.h>

#include "ar_utils.h"
#include "test_server.h"

#include <util/folder/tempdir.h>
#include <util/system/event.h>
#include <util/system/thread.h>

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(StartsAndEndsWithOneOf) {

    Y_UNIT_TEST(TestStartsWithOneOf) {
        UNIT_ASSERT(StartsWithOneOf("abc", "def", TStringBuf("ghi"), TString("ab")));
        UNIT_ASSERT(!StartsWithOneOf("abc"));
        UNIT_ASSERT(!StartsWithOneOf(TString("abc"), "def", TStringBuf("ghi")));
        UNIT_ASSERT(StartsWithOneOf("abc", TString("abc")));
    }

    Y_UNIT_TEST(TestEndsWithOneOf) {
        UNIT_ASSERT(EndsWithOneOf("abc", "def", TStringBuf("ghi"), TString("bc")));
        UNIT_ASSERT(!EndsWithOneOf("abc"));
        UNIT_ASSERT(!EndsWithOneOf(TString("abc"), "def", TStringBuf("ghi")));
        UNIT_ASSERT(EndsWithOneOf("abc", TString("abc")));
    }

}

Y_UNIT_TEST_SUITE(StringBufOperator) {

    Y_UNIT_TEST(Works) {
        TStringBuf s = "1234"_sb;
        UNIT_ASSERT_STRINGS_EQUAL(TStringBuf("1234"), s);
        UNIT_ASSERT_STRINGS_EQUAL(TStringBuf("1234"), "1234"_sb);
    }

}

Y_UNIT_TEST_SUITE(FlushToFileIfNotLocked) {

    Y_UNIT_TEST(TestIfNotLocked) {
        TString testData = "testing";
        TTempDir tmpDir;
        TFsPath filename(tmpDir.Path() / "TestIfNotLocked");
        TMutex mutex;

        FlushToFileIfNotLocked(testData, filename, mutex);

        UNIT_ASSERT_STRINGS_EQUAL(TIFStream(filename).ReadAll(), testData);
    }

    Y_UNIT_TEST(TestIfLocked) {
        class TLockingThread: public ISimpleThread {
        public:
            TLockingThread(TMutex& mutex, TManualEvent& startEvent, TManualEvent& stopEvent)
                : Mutex(mutex)
                , StartEvent(startEvent)
                , StopEvent(stopEvent)
            {
            }
            void* ThreadProc() override {
                Mutex.TryAcquire();
                StartEvent.Signal();

                StopEvent.WaitI();
                Mutex.Release();

                return nullptr;
            }
        private:
            TMutex& Mutex;
            TManualEvent& StartEvent;
            TManualEvent& StopEvent;
        };

        TString testData = "testing";
        TTempDir tmpDir;
        TFsPath filename(tmpDir.Path() / "TestIfLocked");
        TMutex mutex;
        TManualEvent startEvent, stopEvent;

        // Ensure mutex is locked from another thread
        TLockingThread thread(mutex, startEvent, stopEvent);
        thread.Start();
        startEvent.WaitI();

        FlushToFileIfNotLocked(testData, filename, mutex);
        stopEvent.Signal();

        UNIT_ASSERT(!filename.Exists());

        // Ensure mutex is unlocked before destruction
        thread.Join();
    }

}

Y_UNIT_TEST_SUITE(MergeHashMaps) {

    Y_UNIT_TEST(TestWorks) {
        THashMap<TString, TString> hashMap1 = {
            {"a", "1"},
            {"b", "1"},
        };
        THashMap<TString, TString> hashMap2 = {
            {"a", "2"},
            {"c", "2"},
        };

        MergeHashMaps(hashMap1, hashMap2);

        UNIT_ASSERT_STRINGS_EQUAL(hashMap1.at("a"), "2");
        UNIT_ASSERT_STRINGS_EQUAL(hashMap1.at("b"), "1");
        UNIT_ASSERT_STRINGS_EQUAL(hashMap1.at("c"), "2");
    }

}

Y_UNIT_TEST_SUITE(NiceAddrStr) {
    Y_UNIT_TEST(Test) {
        UNIT_ASSERT_STRINGS_EQUAL(NiceAddrStr("77.88.55.88"), "77.88.55.88");
        UNIT_ASSERT_STRINGS_EQUAL(NiceAddrStr("2a02:6b8:c02:412:0:604:df5:d83d"), "2a02:6b8:c02:412:0:604:df5:d83d");
        UNIT_ASSERT_STRINGS_EQUAL(NiceAddrStr(""), "0.0.0.0");
        UNIT_ASSERT_STRINGS_EQUAL(NiceAddrStr("(raw all zeros)"), "0.0.0.0");
        UNIT_ASSERT_STRINGS_EQUAL(NiceAddrStr("(raw 23 43 53 111)"), "0.0.0.0");
        UNIT_ASSERT_STRINGS_EQUAL(NiceAddrStr("0.0.0.0"), "0.0.0.0");
    }
}

Y_UNIT_TEST_SUITE(TestGetRequesterAddress) {
    Y_UNIT_TEST(TestBadSocket) {
        TSocket socket;

        UNIT_ASSERT_STRINGS_EQUAL(GetRequesterAddress(socket), "(raw all zeros)");
    }

    TString GetLocalhostAddress() {
        auto address = TNetworkAddress("localhost", 0);
        return address.Begin()->ai_family == AF_INET6 ? "::1" : "127.0.0.1";
    }

    Y_UNIT_TEST(TestGoodSocket) {
        struct TTestReplier: public TRequestReplier {
            bool DoReply(const TReplyParams&) override {
                return true;
            }
        };

        struct TTestCallBack: public THttpServer::ICallBack {
            TClientRequest* CreateClient() override {
                return new TTestReplier();
            }
        };

        TTestCallBack callBack;
        TTestServer server(callBack);

        TSocket socket(TNetworkAddress(server.Host.HostOrIp, server.Host.Port));

        auto localhostAddress = GetLocalhostAddress();
        UNIT_ASSERT_STRINGS_EQUAL(GetRequesterAddress(socket), localhostAddress);
    }
}

}
