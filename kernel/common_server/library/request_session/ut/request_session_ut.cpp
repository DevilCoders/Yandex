#include <kernel/common_server/library/request_session/request_session.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestRequestSessionSuite) {
    Y_UNIT_TEST(TestBytes) {
        NNeh::THttpRequest request;
        request.SetPostData(TString("simple text request"));
        NUtil::THttpReply response;
        response.SetContent(TString("simple text response"));
        NLogisticProto::TRequestSession proto;
        {
            NExternalAPI::TRequestSessionContainer requestSession = NExternalAPI::THTTPRequestSession::BuildFromRequestResponse(request, response);
            requestSession.SerializeToProto(proto);
            NExternalAPI::TRequestSessionContainer deserializedRequestSession;
            UNIT_ASSERT(deserializedRequestSession.DeserializeFromProto(proto));
        }

        TVector<char> serializedProto;
        const size_t serializedMessageSize = static_cast<size_t>(proto.ByteSize());
        serializedProto.resize(serializedMessageSize);
        const bool ok = proto.SerializeToArray(serializedProto.data(), static_cast<int>(serializedMessageSize));
        UNIT_ASSERT(ok);
        request.SetPostData(TString(serializedProto.data(), serializedProto.size()));
        response.SetContent(TString(serializedProto.data(), serializedProto.size()));
        {
            NExternalAPI::TRequestSessionContainer requestSession = NExternalAPI::THTTPRequestSession::BuildFromRequestResponse(request, response);
            NLogisticProto::TRequestSession proto;
            requestSession.SerializeToProto(proto);
            NExternalAPI::TRequestSessionContainer deserializedRequestSession;
            UNIT_ASSERT(deserializedRequestSession.DeserializeFromProto(proto));
        }
    }
};
