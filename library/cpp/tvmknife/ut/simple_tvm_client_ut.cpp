#include <library/cpp/tvmknife/simple_tvm_client.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTvmknife;

Y_UNIT_TEST_SUITE(SimpleClient) {
    class TTestClient: public TSimpleTvmClient {
    public:
        using TSimpleTvmClient::GetTicketFromResponse;
    };

    Y_UNIT_TEST(GetTicketFromResponse) {
        UNIT_ASSERT_EXCEPTION(TTestClient::GetTicketFromResponse(
                                  R"({"status": "ERR_REQUEST", "error": "MISSING.client_id", "desc": "Arg is required but empty: client_id"})",
                                  127),
                              yexception);

        UNIT_ASSERT_EXCEPTION(TTestClient::GetTicketFromResponse(
                                  R"(   {
                                      "19" : { "ticket" : "service_ticket_1"},
                                      "213" : { "ticket" : "service_ticket_2"},
                                      "234" : { "error" : "Dst is not found" },
                                      "185" : { "ticket" : "service_ticket_3"},
                                      "deprecated" : { "ticket" : "deprecated_ticket" }
                                    })",
                                  127),
                              yexception);

        UNIT_ASSERT_EXCEPTION(TTestClient::GetTicketFromResponse(
                                  R"(   {
                                      "19" : { "ticket" : "service_ticket_1"},
                                      "213" : { "ticket" : "service_ticket_2"},
                                      "234" : { "error" : "Dst is not found" },
                                      "185" : { "ticket" : "service_ticket_3"},
                                      "deprecated" : { "ticket" : "deprecated_ticket" }
                                    })",
                                  234),
                              yexception);

        UNIT_ASSERT_STRINGS_EQUAL("service_ticket_2",
                                  TTestClient::GetTicketFromResponse(
                                      R"(   {
                                          "19" : { "ticket" : "service_ticket_1"},
                                          "213" : { "ticket" : "service_ticket_2"},
                                          "234" : { "error" : "Dst is not found" },
                                          "185" : { "ticket" : "service_ticket_3"},
                                          "deprecated" : { "ticket" : "deprecated_ticket" }
                                        })",
                                      213));
    }
}
