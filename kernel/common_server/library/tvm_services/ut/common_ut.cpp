#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/library/tvm_services/abstract/emulator.h>
#include <kernel/common_server/library/tvm_services/abstract/request/direct.h>
#include <kernel/common_server/obfuscator/obfuscators/abstract.h>
#include <library/cpp/xml/document/xml-document.h>

constexpr const char* TVMCustomization =
R"(
    Token: test
    Type: tvm
)";

constexpr const char* CompositeZoraTvmCustomization =
R"(
    Type: composite
    <zora>
        Type: zora
        DestUrl: http://localhost/with/path/
        ClientId: zora_unit_test
    </zora>
    <tvm>
        Token: test
        Type: tvm
    </tvm>
)";

constexpr const char* PersonalData = "secretekey";
constexpr const char* ObfuscatedMessage = "obfuscated";

static TString BuildConfigString(TMaybe<ui16> serverPort, const char* customization = TVMCustomization) {
    TStringStream ss;
    if (serverPort.Defined()) {
        ss << "ApiHost: localhost\n"
            << "ApiPort: " << serverPort << "\n"
            << "Https: 0\n";
    } else {
        ss << "ApiHost: \n"
            << "ApiPort: 80\n"
            << "Https: 0\n";
    }
    ss << "RequestTimeout: 10s\n";
    ss << "<RequestConfig>\n"
        << "MaxAttempts: 1\n"
        << "TimeoutSendingms: 8000\n"
        << "TimeoutConnectms: 5000\n"
        << "GlobalTimeout: 5000\n"
        << "TasksCheckInterval: 5000\n"
        << "</RequestConfig>\n";
    ss << "<Customization>\n"
        << customization
        << "</Customization>\n";
    ss << "DestinationClientId: 1\n";
    ss << "SelfClientId: 1\n";
    ss << "CommonPrefix:\n";

    return ss.Str();
}

template <class TConfigType>
TConfigType GetClientConfigSimple(const ui16 serverPort, const char* customization = TVMCustomization) {
    auto configStr = BuildConfigString(serverPort, customization);
    TConfigType result;
    TAnyYandexConfig config;
    CHECK_WITH_LOG(config.ParseMemory(configStr.data()));
    result.Init(config.GetRootSection(), nullptr);
    return result;
}
Y_UNIT_TEST_SUITE(DefaultSuite) {

    Y_UNIT_TEST(MultipartParser) {
        class TTestReplyConstructor: public IReplyConstructor {
        public:
            bool GetReply(const TString& /*path*/, const NJson::TJsonValue& /*postJson*/, const TCgiParameters& /*cgi*/, THttpHeaders& headers, TString& content) const override {
                headers.AddHeader("Content-Type", "multipart/related; type=\"application/xop+xml\"; boundary=\"uuid:2942f834-b58c-4176-88e9-70a8b9c76260\"; start=\"<root.message@cxf.apache.org>\"; start-info=\"text/xml\";charset=UTF-8");
                content = R"(--uuid:2942f834-b58c-4176-88e9-70a8b9c76260
Content-Disposition: form-data; name="submit-name"
Content-Type: application/xop+xml; charset=UTF-8; type="text/xml"
Content-Transfer-Encoding: binary
Content-ID: <root.message@cxf.apache.org>

<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"><soap:Body><soap:Fault><faultcode>soap:Server</faultcode><faultstring>&#x421;&#x43e;&#x43e;&#x431;&#x449;&#x435;&#x43d;&#x438;&#x435; &#x441;&#x43e;&#x434;&#x435;&#x440;&#x436;&#x438;&#x442; &#x43d;&#x435; &#x432;&#x441;&#x435; &#x432;&#x43b;&#x43e;&#x436;&#x435;&#x43d;&#x43d;&#x44b;&#x435; &#x44d;&#x43b;&#x435;&#x43c;&#x435;&#x43d;&#x442;&#x44b;. &#x411;&#x43b;&#x43e;&#x43a; CallerInformationSystemSignature &#x43e;&#x442;&#x441;&#x443;&#x442;&#x441;&#x442;&#x432;&#x443;&#x435;&#x442; &#x43b;&#x438;&#x431;&#x43e; &#x43f;&#x443;&#x441;&#x442;.</faultstring><detail><ns3:InvalidContent xmlns:ns3="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/faults/1.2" xmlns:ns2="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/1.2" xmlns="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/basic/1.2"><Code>tsmev3:PRODUCTION_AREA:TSMEV3_CORE2 : TR:SYNC:PP:55</Code><Description>SMEV-403:&#x421;&#x43e;&#x43e;&#x431;&#x449;&#x435;&#x43d;&#x438;&#x435; &#x441;&#x43e;&#x434;&#x435;&#x440;&#x436;&#x438;&#x442; &#x43d;&#x435; &#x432;&#x441;&#x435; &#x432;&#x43b;&#x43e;&#x436;&#x435;&#x43d;&#x43d;&#x44b;&#x435; &#x44d;&#x43b;&#x435;&#x43c;&#x435;&#x43d;&#x442;&#x44b;. &#x411;&#x43b;&#x43e;&#x43a; CallerInformationSystemSignature &#x43e;&#x442;&#x441;&#x443;&#x442;&#x441;&#x442;&#x432;&#x443;&#x435;&#x442; &#x43b;&#x438;&#x431;&#x43e; &#x43f;&#x443;&#x441;&#x442;.</Description></ns3:InvalidContent></detail></soap:Fault></soap:Body></soap:Envelope>
--uuid:2942f834-b58c-4176-88e9-70a8b9c76260--)";
                return true;
            }
        };

        class TTestRequest: public NExternalAPI::IServiceApiHttpRequest {
        protected:
        public:
            bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
                request.SetUri("test");
                return true;
            }

            class TResponse: public IResponseByContentType {
            private:
                CS_ACCESS(TResponse, ui32, StartsCount, 0);
                CSA_DEFAULT(TResponse, TString, PartName);
            protected:
                virtual bool DoStart(const NExternalAPI::TContentPartContext& context) override {
                    PartName = context.GetPartName();
                    if (StartsCount == 0) {
                        CHECK_WITH_LOG(context.GetContentType() == NExternalAPI::EContentType::Multipart);
                        CHECK_WITH_LOG(!PartName);
                    } else {
                        CHECK_WITH_LOG(context.GetContentType() == NExternalAPI::EContentType::XML);
                        CHECK_WITH_LOG(PartName == "submit-name");
                    }
                    ++StartsCount;
                    return true;
                }
                virtual bool DoParseXML(const NXml::TDocument& doc) override {
                    TFLEventLog::Info("reply")("content", doc.Root().ToString(), 100000);
                    UNIT_ASSERT(doc.Root().ToString().StartsWith("<soap:Envelope"));
                    UNIT_ASSERT(doc.Root().ToString().EndsWith("soap:Envelope>"));
                    return true;
                }
            };
        };

        ui16 serverPort = Singleton<TPortManager>()->GetPort();
        TBaseEmulatorServer emulatorServer;
        emulatorServer.RegisterReplier("test", MakeAtomicShared<TTestReplyConstructor>());
        emulatorServer.Run(serverPort);

        auto clientConfig = GetClientConfigSimple<NExternalAPI::TSenderConfig>(serverPort);
        NExternalAPI::TSender sender(clientConfig, "test");
        TTestRequest::TResponse reply = sender.SendRequest<TTestRequest>();
        UNIT_ASSERT(reply.IsSuccess());
    }

    Y_UNIT_TEST(TVMCustomization) {
        class TTestRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        public:
            bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
                request.SetUri("test");
                return true;
            }

            class TResponse: public IHttpRequestWithJsonReport::TJsonResponse {
                bool DoParseJsonReply(const NJson::TJsonValue& jsonReply) override {
                    Y_UNUSED(jsonReply);
                    return true;
                }
            };
        };

        class TTestReplyConstructor: public IReplyConstructor {
        public:
            bool GetReply(const TString& /*path*/, const NJson::TJsonValue& /*postJson*/, const TCgiParameters& /*cgi*/, THttpHeaders& /*headers*/, TString& content) const override {
                content = "{}";
                return true;
            }
        };

        ui16 serverPort = Singleton<TPortManager>()->GetPort();
        TBaseEmulatorServer emulatorServer;
        emulatorServer.RegisterReplier("test", MakeAtomicShared<TTestReplyConstructor>());
        emulatorServer.Run(serverPort);

        auto clientConfig = GetClientConfigSimple<NExternalAPI::TSenderConfig>(serverPort);
        NExternalAPI::TSender sender(clientConfig, "test");
        NNeh::THttpRequest request;
        NExternalAPI::TServiceApiHttpDirectRequest reqOriginal;
        UNIT_ASSERT(sender.TuneRequest(reqOriginal, request));
        UNIT_ASSERT(request.GetHeaders().contains("Authorization"));
        TTestRequest::TResponse reply = sender.SendRequest<TTestRequest>();
        UNIT_ASSERT(reply.IsSuccess());
    }

    Y_UNIT_TEST(CompositeZoraTvmCustomization) {
        class TTestRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        public:
            bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
                request.AddCgiData("added_cgi", "added_value_1");
                request.AddCgiData("added_cgi2", "added_value_2");
                request.SetPostData(TString{R"({"key":"value", "key2": 123})"});
                return true;
            }

            class TResponse: public IHttpRequestWithJsonReport::TJsonResponse {
                bool DoParseJsonReply(const NJson::TJsonValue& /*jsonReply*/) override {
                    return true;
                }
            };
        };

        class TTestReplyConstructor: public IReplyConstructor {
        public:
            bool GetReply(const TString& /*path*/, const NJson::TJsonValue& /*postJson*/, const TCgiParameters& /*cgi*/, THttpHeaders& /*headers*/, TString& content) const override {
                content = "{}";
                return true;
            }
        };

        ui16 serverPort = Singleton<TPortManager>()->GetPort();
        TBaseEmulatorServer emulatorServer;
        emulatorServer.RegisterReplier("", MakeAtomicShared<TTestReplyConstructor>());
        emulatorServer.Run(serverPort);

        auto clientConfig = GetClientConfigSimple<NExternalAPI::TSenderConfig>(serverPort, CompositeZoraTvmCustomization);
        NExternalAPI::TSender sender(clientConfig, "");

        {
            TTestRequest request;
            NNeh::THttpRequest httpRequest;
            UNIT_ASSERT(sender.PrepareNehRequest(request, httpRequest));
            UNIT_ASSERT_EQUAL_C(httpRequest.GetRequest(), "", ". Request: \"" << httpRequest.GetRequest() << "\".");
            UNIT_ASSERT(httpRequest.GetHeaders().contains("Authorization"));
            UNIT_ASSERT(httpRequest.GetHeaders().contains("X-Ya-Dest-Url"));
            UNIT_ASSERT_EQUAL_C(httpRequest.GetHeaders().at("X-Ya-Dest-Url"), "http://localhost/with/path/?added_cgi=added_value_1&added_cgi2=added_value_2", ". Actual X-Ya-Dest-Url value: \"" << httpRequest.GetHeaders().at("X-Ya-Dest-Url") << "\".");
            UNIT_ASSERT_EQUAL(httpRequest.GetPostData().AsStringBuf(), R"({"key":"value", "key2": 123})");
        }

        TTestRequest request;
        TTestRequest::TResponse reply = sender.SendRequest(request);
        UNIT_ASSERT(reply.IsSuccess());
    }

    Y_UNIT_TEST(ObfuscateLog) {
        class TTestRequest: public NExternalAPI::IHttpRequestWithJsonReport {
        public:
            bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
                request.SetUri("test");
                NJson::TJsonValue bodyJson;
                JWRITE(bodyJson, "some_key", PersonalData);
                request.SetPostData(bodyJson);
                return true;
            }

            class TResponse: public IHttpRequestWithJsonReport::TJsonResponse {
                bool DoParseJsonReply(const NJson::TJsonValue& jsonReply) override {
                    Y_UNUSED(jsonReply);
                    return true;
                }
            };
        };

        class TTestReplyConstructor: public IReplyConstructor {
        public:
            bool GetReply(const TString& /*path*/, const NJson::TJsonValue& /*postJson*/, const TCgiParameters& /*cgi*/, THttpHeaders& /*headers*/, TString& content) const override {
                content = PersonalData;
                return true;
            }
        };



        class TTestObfuscatorManager : public NCS::NObfuscator::IObfuscatorManager {
        private:
            class TTestObfuscator : public NCS::NObfuscator::IObfuscator {
            private:
                TMap<TString, TString> EmptyRules = {};

                virtual TString DoObfuscate(const TStringBuf /*str*/) const override {
                    return ObfuscatedMessage;
                }
            public:
                virtual TString GetClassName() const override {
                    return "test_obfuscator";
                }
                virtual size_t GetRulesCount() const override {
                    return 0;
                }
                virtual bool IsMatch(const NCS::NObfuscator::TObfuscatorKey& /*key*/) const override {
                    return true;
                }
                virtual NJson::TJsonValue DoSerializeToJson() const override {
                    return {};
                }
                virtual bool DoDeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) override {
                    return true;
                }
                virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const override {
                    return NFrontend::TScheme();
                }
            };

        public:
            virtual NCS::NObfuscator::IObfuscator::TPtr GetObfuscatorFor(const NCS::NObfuscator::TObfuscatorKey& /*key*/) const override {
                return MakeAtomicShared<TTestObfuscator>();
            }

            virtual bool Start() noexcept override {
                return true;
            }
            virtual bool Stop() noexcept override {
                return true;
            }

        };

        class TTestRequestCustomizationContext : public NExternalAPI::IRequestCustomizationContext {
            virtual TMaybe<NSimpleMeta::TConfig> GetRequestConfig(const TString& /*apiName*/, const NNeh::THttpRequest& /*request*/) const override {
                return Nothing();
            }
            virtual NCS::NObfuscator::TObfuscatorManagerContainer GetObfuscatorManager() const override {
                return MakeAtomicShared<TTestObfuscatorManager>();
            }
        };

        ui16 serverPort = Singleton<TPortManager>()->GetPort();
        TBaseEmulatorServer emulatorServer;
        emulatorServer.RegisterReplier("test", MakeAtomicShared<TTestReplyConstructor>());
        emulatorServer.Run(serverPort);

        auto clientConfig = GetClientConfigSimple<NExternalAPI::TSenderConfig>(serverPort);
        auto customizationConfig = TTestRequestCustomizationContext();
        TFLEventLog::TContextGuard logContext;
        clientConfig.MutableCustomizer() = nullptr;
        NExternalAPI::TSender sender(clientConfig, "test", &customizationConfig);
        NNeh::THttpRequest request;
        NExternalAPI::TServiceApiHttpDirectRequest reqOriginal;
        UNIT_ASSERT(sender.TuneRequest(reqOriginal, request));
        TTestRequest::TResponse reply = sender.SendRequest<TTestRequest>();
        TString str = logContext.GetStringReport();
        UNIT_ASSERT(str.find(PersonalData) == TString::npos);
        UNIT_ASSERT(str.find(ObfuscatedMessage) != TString::npos);
    }

}
