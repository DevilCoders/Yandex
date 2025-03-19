#include <kernel/turbo/login/lib/turbo_login.h>

#include <apphost/lib/service_testing/service_testing.h>
#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/json/json_reader.h>


const TStringBuf HTTP_ADAPTER_REQUEST_TEMPLATE = R"([
    {
        "name": "HTTP_REQUEST",
        "results": [
            {
                "headers": [
                    [
                        "Host",
                        "yandex.ru"
                    ],
                    [
                        "Host",
                        "yandex.ru"
                    ],
                    [
                        "X-Forwarded-For",
                        "178.120.68.232"
                    ],
                    [
                        "Accept",
                        "*/*"
                    ],
                    [
                        "X-HTTPS-Request",
                        "yes"
                    ],
                    [
                        "X-Yandex-HTTPS",
                        "yes"
                    ],
                    [
                        "Yandex-Turbo-Status",
                        "auto"
                    ],
                    [
                        "Yandex-Net-Info",
                        "network_type=wifi; rtt=103; bandwidth=382;"
                    ],
                    [
                        "User-Agent",
                        "Mozilla/5.0 (Linux; Android 6.0.1; Redmi 3S Build/MMB29M) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 YaBrowser/17.10.1.370.00 Mobile Safari/537.36"
                    ],
                    [
                        "Accept-Language",
                        "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4"
                    ],
                    [
                        "X-Yandex-Req-Id",
                        "1549277351202637-1274705625791136109800034-sas1-5505"
                    ]
                ],
                "uri": "/turbo/login?domain=hurma.com.au",
                "type": "http_request",
                "request_time": 1541170606,
                "remote_ip": "2a02:6b8:0:80b::5f6c:8e05",
                "method": "GET",
                "port": 31815,
                "content": ""
            }
        ]
    }
])";


Y_UNIT_TEST_SUITE(TurboLoginErrorsCheck) {
    Y_UNIT_TEST(CheckNoHttps) {
        const TStringBuf NOT_HTTPS_REQUEST_TEMPLATE = R"([
            {
                "name": "HTTP_REQUEST",
                "results": [
                    {
                        "headers": [
                            [
                                "Host",
                                "yandex.ru"
                            ],
                            [
                                "Accept",
                                "*/*"
                            ],
                            [
                                "Yandex-Turbo-Status",
                                "auto"
                            ],
                            [
                                "Yandex-Net-Info",
                                "network_type=wifi; rtt=103; bandwidth=382;"
                            ],
                            [
                                "User-Agent",
                                "Mozilla/5.0 (Linux; Android 6.0.1; Redmi 3S Build/MMB29M) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 YaBrowser/17.10.1.370.00 Mobile Safari/537.36"
                            ],
                            [
                                "Accept-Language",
                                "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4"
                            ]
                        ],
                        "uri": "/turbo/login?domain=hurma.com.au",
                        "type": "http_request",
                        "request_time": 1541170606,
                        "remote_ip": "2a02:6b8:0:80b::5f6c:8e05",
                        "method": "GET",
                        "port": 31815,
                        "content": ""
                    }
                ]
            }
        ])";
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(NOT_HTTPS_REQUEST_TEMPLATE, &httpAdapterRequest));

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {1, 2, 3, 4, 5 };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 301);
    }

    Y_UNIT_TEST(CheckOptions) {
        const TStringBuf NOT_HTTPS_REQUEST_TEMPLATE = R"([
            {
                "name": "HTTP_REQUEST",
                "results": [
                    {
                        "headers": [
                            [
                                "Host",
                                "yandex.ru"
                            ],
                            [
                                "Accept",
                                "*/*"
                            ],
                            [
                                "Yandex-Turbo-Status",
                                "auto"
                            ],
                            [
                                "Referer",
                                "https://yandex.com/turbo?text=page1.com"
                            ],
                            [
                                "Yandex-Net-Info",
                                "network_type=wifi; rtt=103; bandwidth=382;"
                            ],
                            [
                                "User-Agent",
                                "Mozilla/5.0 (Linux; Android 6.0.1; Redmi 3S Build/MMB29M) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 YaBrowser/17.10.1.370.00 Mobile Safari/537.36"
                            ],
                            [
                                "Accept-Language",
                                "ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4"
                            ],
                            [
                                "X-Yandex-HTTPS",
                                "yes"
                            ]
                        ],
                        "uri": "/turbo/login?domain=hurma.com.au",
                        "type": "http_request",
                        "request_time": 1541170606,
                        "remote_ip": "2a02:6b8:0:80b::5f6c:8e05",
                        "method": "OPTIONS",
                        "port": 31815,
                        "content": ""
                    }
                ]
            }
        ])";
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(NOT_HTTPS_REQUEST_TEMPLATE, &httpAdapterRequest));

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {1, 2, 3, 4, 5 };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 200);

        TString allowOrigin;
        TString accessControlAllowMethods;
        TString accessControlAllowHeaders;
        bool allowCredentials = false;
        for (const auto& header : (*httpResponse)["headers"].GetArray()) {
            if (header[0] == "Access-Control-Allow-Origin") {
                allowOrigin = header[1].GetString();
            } else if (header[0] == "Access-Control-Allow-Methods") {
                accessControlAllowMethods = header[1].GetString();
            } else if (header[0] == "Access-Control-Allow-Credentials") {
                allowCredentials = header[1].GetBooleanRobust();
            } else if (header[0] == "Access-Control-Allow-Headers") {
                accessControlAllowHeaders = header[1].GetString();
            }
        }
        UNIT_ASSERT(allowCredentials);
        UNIT_ASSERT_EQUAL(allowOrigin, "https://yandex.com");
        UNIT_ASSERT_EQUAL(accessControlAllowMethods, TString("GET, OPTIONS"));
        UNIT_ASSERT_EQUAL(accessControlAllowHeaders, TString("*"));
    }

    Y_UNIT_TEST(CheckNoDomain) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));
        httpAdapterRequest[0]["results"][0]["uri"] = "/turbo/login";

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {1, 2, 3, 4, 5 };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 422);
    }

    Y_UNIT_TEST(CheckMultipleDomainCgis) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));
        httpAdapterRequest[0]["results"][0]["uri"] = "/turbo/login?domain=australia.gov&domain=indonesia.gov";

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {1, 2, 3, 4, 5 };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 422);
    }

    Y_UNIT_TEST(CheckTooLongDomain) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));
        TString hackerGeneratedDomain;
        hackerGeneratedDomain.resize(10000, '1');
        hackerGeneratedDomain += ".com";
        httpAdapterRequest[0]["results"][0]["uri"] = TString("/turbo/login?domain=") + hackerGeneratedDomain;

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {1, 2, 3, 4, 5 };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 422);
    }

    Y_UNIT_TEST(NonAuthorizedDomain) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));
        NJson::TJsonValue refererHeader;
        refererHeader.AppendValue("Referer");
        refererHeader.AppendValue("not-yanndex.com");
        httpAdapterRequest[0]["results"][0]["headers"].AppendValue(std::move(refererHeader));

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {1, 2, 3, 4, 5 };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 403);
        TString allowOrigin;
        for (const auto& header : (*httpResponse)["headers"].GetArray()) {
            if (header[0] == "Access-Control-Allow-Origin") {
                allowOrigin = header[1].GetString();
                break;
            }
        }
        UNIT_ASSERT(allowOrigin.empty());
    }
}


Y_UNIT_TEST_SUITE(TurboLogin) {
    Y_UNIT_TEST(UnauthorizedUser) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));

        NJson::TJsonValue cookieHeader;
        cookieHeader.AppendValue("Cookie");
        cookieHeader.AppendValue("yandexuid=1234");
        httpAdapterRequest[0]["results"][0]["headers"].AppendValue(std::move(cookieHeader));
        NJson::TJsonValue refererHeader;
        refererHeader.AppendValue("Referer");
        refererHeader.AppendValue("https://yandex.com/turbo?text=page1.com");
        httpAdapterRequest[0]["results"][0]["headers"].AppendValue(std::move(refererHeader));

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {'1', '2', '3', '4', '5' };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 200);

        bool allowCredentials = false;
        TString accessControlAllowHeaders;
        TString allowOrigin;
        for (const auto& header : (*httpResponse)["headers"].GetArray()) {
            if (header[0] == "Access-Control-Allow-Credentials") {
                allowCredentials = header[1].GetBooleanRobust();
            } else if (header[0] == "Access-Control-Allow-Headers") {
                accessControlAllowHeaders = header[1].GetString();
            } else if (header[0] == "Access-Control-Allow-Origin") {
                allowOrigin = header[1].GetString();
            }
        }
        UNIT_ASSERT(allowCredentials);
        UNIT_ASSERT_EQUAL(accessControlAllowHeaders, TString("*"));
        UNIT_ASSERT_EQUAL(allowOrigin, "https://yandex.com");

        // 'u:' + hmac.new(key='12345', msg='hurma.com.au:1234', digestmod=hashlib.sha256).hexdigest()
        NJson::TJsonValue resultContent;
        UNIT_ASSERT(NJson::ReadJsonTree((*httpResponse)["content"].GetString(), &resultContent));
        UNIT_ASSERT_EQUAL(resultContent["turbo_uid"].GetString(), "u:db936f0a2952435990cb04b772d962970e5c83a226a45f11c0ff7ff502535470");
    }

    Y_UNIT_TEST(CheckNoCookiesWithRandomYandexUid) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));

        NJson::TJsonValue yandexRandomUidHeader;
        yandexRandomUidHeader.AppendValue("X-Yandex-RandomUID");
        yandexRandomUidHeader.AppendValue("2113610981549277351");
        httpAdapterRequest[0]["results"][0]["headers"].AppendValue(std::move(yandexRandomUidHeader));

        NJson::TJsonValue yandexReqidHeader;
        yandexReqidHeader.AppendValue( "X-Yandex-Req-Id");
        yandexReqidHeader.AppendValue("1549277351202637-1274705625791136109800034-sas1-5505");
        httpAdapterRequest[0]["results"][0]["headers"].AppendValue(std::move(yandexReqidHeader));

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {'1', '2', '3', '4', '5' };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 200);
         // 'u:' + hmac.new(key='12345', msg='hurma.com.au:2113610981549277351', digestmod=hashlib.sha256).hexdigest()
        NJson::TJsonValue resultContent;
        UNIT_ASSERT(NJson::ReadJsonTree((*httpResponse)["content"].GetString(), &resultContent));
        UNIT_ASSERT_EQUAL(resultContent["turbo_uid"].GetString(), "u:db510584deb2d3d67df2af9c275b1a2efe94d702b510c608bb65f16c561b7a49");

        bool containsSetYandexUidCookie = false;
        bool allowCredentials = false;
        TString accessControlAllowHeaders;
        for (const auto& header : (*httpResponse)["headers"].GetArray()) {
            if ((header[0] == "Set-Cookie") && (header[1].GetString().Contains("yandexuid=2113610981549277351"))) {
                containsSetYandexUidCookie = true;
            }
            if (header[0] == "Access-Control-Allow-Credentials") {
                allowCredentials = header[1].GetBooleanRobust();
            } else if (header[0] == "Access-Control-Allow-Headers") {
                accessControlAllowHeaders = header[1].GetString();
            }
        }

        UNIT_ASSERT(allowCredentials);
        UNIT_ASSERT(containsSetYandexUidCookie);
        UNIT_ASSERT_EQUAL(accessControlAllowHeaders, TString("*"));
    }

    Y_UNIT_TEST(CheckNoCookiesNoRandomYandexUid) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));

        NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
        const TVector<ui8> signatureSecretKey = {1, 2, 3, 4, 5 };
        NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

        const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
        UNIT_ASSERT(blackboxHttpRequests.empty());

        const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
        UNIT_ASSERT(httpResponse != nullptr);
        UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 200);
        NJson::TJsonValue resultContent;
        UNIT_ASSERT(NJson::ReadJsonTree((*httpResponse)["content"].GetString(), &resultContent));
        UNIT_ASSERT(resultContent["turbo_uid"].GetString().StartsWith("u:"));

        bool containsSetYandexUidCookie = false;
        bool allowCredentials = false;
        TString accessControlAllowHeaders;
        for (const auto& header : (*httpResponse)["headers"].GetArray()) {
            if ((header[0] == "Set-Cookie") && (header[1].GetString().Contains("yandexuid"))) {
                containsSetYandexUidCookie = true;
                break;
            }
            if (header[0] == "Access-Control-Allow-Credentials") {
                allowCredentials = header[1].GetBooleanRobust();
            } else if (header[0] == "Access-Control-Allow-Headers") {
                accessControlAllowHeaders = header[1].GetString();
            }
        }

        UNIT_ASSERT(allowCredentials);
        UNIT_ASSERT(containsSetYandexUidCookie);
        UNIT_ASSERT_EQUAL(accessControlAllowHeaders, TString("*"));
    }

    Y_UNIT_TEST(AuthorizedUser) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));
        httpAdapterRequest[0]["results"][0]["headers"][0][1] = "https://mvidio-ru.turbopages.org";

        NJson::TJsonValue cookieHeader;
        cookieHeader.AppendValue("Cookie");
        cookieHeader.AppendValue("news_lang=ru; Session_id=54321; yandexuid=1234");
        httpAdapterRequest[0]["results"][0]["headers"].AppendValue(std::move(cookieHeader));
        NJson::TJsonValue refererHeader;
        refererHeader.AppendValue("Referer");
        refererHeader.AppendValue("https://yandex.com/turbo?text=page1.com");
        httpAdapterRequest[0]["results"][0]["headers"].AppendValue(std::move(refererHeader));

        const TVector<ui8> signatureSecretKey = {'1', '2', '3', '4', '5' };

        {
            // Test for different hosts.
            for (const TString& host : {"", "yandex.ru", "yandex.by"}) {
                auto httpAdapterRequestWithCustomHost = httpAdapterRequest;
                // For empty host we expect to have yandex.ru as a fallback.
                TString expectedHost = (host.empty()) ? "yandex.ru" : host;
                // Append new host to the context.
                NJson::TJsonValue hostHeader;
                hostHeader.AppendValue("Host");
                hostHeader.AppendValue(host);
                httpAdapterRequestWithCustomHost[0]["results"][0]["headers"].AppendValue(std::move(hostHeader));
                NAppHost::NService::TTestContext testCtx {httpAdapterRequestWithCustomHost};
                NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

                UNIT_ASSERT(testCtx.FindFirstItem("http_response") == nullptr);

                const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
                UNIT_ASSERT(!blackboxHttpRequests.empty());

                NAppHostHttp::THttpRequest httpRequest;
                UNIT_ASSERT(blackboxHttpRequests.begin()->Fill(&httpRequest));

                UNIT_ASSERT(NAppHostHttp::THttpRequest::Get == httpRequest.GetMethod());
                UNIT_ASSERT(NAppHostHttp::THttpRequest::Http == httpRequest.GetScheme());
                UNIT_ASSERT_VALUES_EQUAL(httpRequest.GetPath(),
                    "blackbox?method=sessionid&format=json&sessionid=54321&userip=178.120.68.232&host=" + expectedHost);
            }
        }

        {
            const TString blackboxResponseStr = R"(
                {
                    "age": 88,
                    "expires_in": 7775912,
                    "ttl": "5",
                    "error": "OK",
                    "status": {
                        "value": "VALID",
                        "id": 0
                    },
                    "uid": {
                        "value": "1234567",
                        "lite": false,
                        "hosted": false
                    },
                    "login": "turbo",
                    "have_password": true,
                    "have_hint": false,
                    "karma": {
                        "value": 0
                    },
                    "karma_status": {
                        "value": 0
                    },
                    "auth": {
                        "password_verification_age": 88,
                        "have_password": true,
                        "secure": true,
                        "partner_pdd_token": false
                    },
                    "connection_id": "***"
                }
            )";
            NAppHostHttp::THttpResponse blackboxHttpResponse;
            blackboxHttpResponse.SetStatusCode(200);
            blackboxHttpResponse.SetContent(blackboxResponseStr);

            NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
            testCtx.AddProtobufItem(blackboxHttpResponse, "blackbox_http_response", NAppHost::EContextItemKind::Input);

            NTurboLogin::ProcessTurboLoginWithBlackbox(testCtx, signatureSecretKey);

            const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
            UNIT_ASSERT(httpResponse != nullptr);
            UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 200);

            bool allowCredentials = false;
            TString accessControlAllowHeaders, allowOrigin;
            for (const auto& header : (*httpResponse)["headers"].GetArray()) {
                if (header[0] == "Access-Control-Allow-Credentials") {
                    allowCredentials = header[1].GetBooleanRobust();
                } else if (header[0] == "Access-Control-Allow-Headers") {
                    accessControlAllowHeaders = header[1].GetString();
                } else if (header[0] == "Access-Control-Allow-Origin") {
                    allowOrigin = header[1].GetString();
                }
            }
            UNIT_ASSERT(allowCredentials);
            UNIT_ASSERT_EQUAL(accessControlAllowHeaders, TString("*"));
            UNIT_ASSERT_EQUAL(allowOrigin, "https://yandex.com");

            // 'a:' + hmac.new(key='12345', msg='hurma.com.au:1234567', digestmod=hashlib.sha256).hexdigest()
            NJson::TJsonValue resultContent;
            UNIT_ASSERT(NJson::ReadJsonTree((*httpResponse)["content"].GetString(), &resultContent));
            UNIT_ASSERT_EQUAL(resultContent["turbo_uid"].GetString(), "a:17fb0bbb2938583a2bea8ce028bd1d3ae987b284cb3e65875e76533bdf1f84ce");
        }
    }

    Y_UNIT_TEST(AuthorizedUserWithExpiredLogin) {
        NJson::TJsonValue httpAdapterRequest;
        UNIT_ASSERT(NJson::ReadJsonTree(HTTP_ADAPTER_REQUEST_TEMPLATE, &httpAdapterRequest));

        NJson::TJsonValue cookieHeader;
        cookieHeader.AppendValue("Cookie");
        cookieHeader.AppendValue("news_lang=ru; Session_id=54321; yandexuid=1234");
        httpAdapterRequest[0]["results"][0]["headers"].AppendValue(std::move(cookieHeader));

        const TVector<ui8> signatureSecretKey = {'1', '2', '3', '4', '5' };

        {
            NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
            NTurboLogin::ProcessTurboLogin(testCtx, signatureSecretKey);

            UNIT_ASSERT(testCtx.FindFirstItem("http_response") == nullptr);

            const auto& blackboxHttpRequests = testCtx.GetProtobufItemRefs("blackbox_http_request");
            UNIT_ASSERT(!blackboxHttpRequests.empty());

            NAppHostHttp::THttpRequest httpRequest;
            UNIT_ASSERT(blackboxHttpRequests.begin()->Fill(&httpRequest));

            UNIT_ASSERT(NAppHostHttp::THttpRequest::Get == httpRequest.GetMethod());
            UNIT_ASSERT(NAppHostHttp::THttpRequest::Http == httpRequest.GetScheme());
            UNIT_ASSERT_VALUES_EQUAL(httpRequest.GetPath(),
                "blackbox?method=sessionid&format=json&sessionid=54321&userip=178.120.68.232&host=yandex.ru");
        }

        {
            const TString blackboxResponseStr = R"(
                {
                    "age": 88,
                    "expires_in": 7775912,
                    "ttl": "5",
                    "error": "OK",
                    "status": {
                        "value": "EXPIRED",
                        "id": 0
                    },
                    "uid": {
                        "value": "1234567",
                        "lite": false,
                        "hosted": false
                    },
                    "login": "turbo",
                    "have_password": true,
                    "have_hint": false,
                    "karma": {
                        "value": 0
                    },
                    "karma_status": {
                        "value": 0
                    },
                    "auth": {
                        "password_verification_age": 88,
                        "have_password": true,
                        "secure": true,
                        "partner_pdd_token": false
                    },
                    "connection_id": "***"
                }
            )";
            NAppHostHttp::THttpResponse blackboxHttpResponse;
            blackboxHttpResponse.SetStatusCode(200);
            blackboxHttpResponse.SetContent(blackboxResponseStr);

            NAppHost::NService::TTestContext testCtx {httpAdapterRequest};
            testCtx.AddProtobufItem(blackboxHttpResponse, "blackbox_http_response", NAppHost::EContextItemKind::Input);

            NTurboLogin::ProcessTurboLoginWithBlackbox(testCtx, signatureSecretKey);

            const NJson::TJsonValue* httpResponse = testCtx.FindFirstItem("http_response");
            UNIT_ASSERT(httpResponse != nullptr);
            UNIT_ASSERT_EQUAL((*httpResponse)["status_code"].GetIntegerRobust(), 200);

            bool allowCredentials = false;
            TString accessControlAllowHeaders;
            for (const auto& header : (*httpResponse)["headers"].GetArray()) {
                if (header[0] == "Access-Control-Allow-Credentials") {
                    allowCredentials = header[1].GetBooleanRobust();
                } else if (header[0] == "Access-Control-Allow-Headers") {
                    accessControlAllowHeaders = header[1].GetString();
                }
            }
            UNIT_ASSERT(allowCredentials);
            UNIT_ASSERT_EQUAL(accessControlAllowHeaders, TString("*"));

            // 'u:' + hmac.new(key='12345', msg='hurma.com.au:1234', digestmod=hashlib.sha256).hexdigest()
            NJson::TJsonValue resultContent;
            UNIT_ASSERT(NJson::ReadJsonTree((*httpResponse)["content"].GetString(), &resultContent));
            UNIT_ASSERT_EQUAL(resultContent["turbo_uid"].GetString(), "u:db936f0a2952435990cb04b772d962970e5c83a226a45f11c0ff7ff502535470");
        }
    }
}
