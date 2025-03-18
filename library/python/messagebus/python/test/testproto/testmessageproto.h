#pragma once

#include <library/python/messagebus/python/test/testproto/testmessage.pb.h>

#include <library/cpp/messagebus/protobuf/ybusbuf.h>

namespace NPyMessageBusTest {
    enum {
        MTYPE_TESTMESSAGE = 42,
    };

    using TBusTestMessage = NBus::TBusBufferMessage<TTestMessage, MTYPE_TESTMESSAGE>;

    class TTestProtocol: public NBus::TBusBufferProtocol {
    public:
        TTestProtocol()
            : NBus::TBusBufferProtocol("TEST", 424242)
        {
            RegisterType(new TBusTestMessage);
        }

        NBus::TBusKey GetKey(const NBus::TBusMessage*) override {
            return NBus::YBUS_KEYLOCAL;
        }
    };

}
