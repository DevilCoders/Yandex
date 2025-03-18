#include "wrapper.h"

#include "testmessageproto.h"

namespace NPyMessageBusTest {
    NMessageBusWrapper::TProtocol TestProtocol() {
        return NMessageBusWrapper::TProtocol(new NPyMessageBusTest::TTestProtocol);
    }
}
