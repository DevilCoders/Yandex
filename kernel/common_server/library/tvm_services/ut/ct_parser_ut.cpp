#include <kernel/common_server/library/tvm_services/abstract/request/ct_parser.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {
    class TCallback : public NExternalAPI::IParserCallbackContext {
        virtual bool DoStart(const NExternalAPI::TContentPartContext& /*context*/) override {
            return true;
        }
        virtual bool DoFinish() override {
            return true;
        }
        virtual bool DoParseXML(const NXml::TDocument& /*doc*/) override {
            return true;
        }

        virtual bool DoParseJson(const NJson::TJsonValue& /*jsonInfo*/) override {
            return true;
        }

        virtual bool DoParseText(const TStringBuf /*text*/) override {
            return true;
        }

        virtual bool DoParseBinary(const TStringBuf /*data*/) override {
            return true;
        }

        virtual bool DoParseZip(const TStringBuf /*data*/) override {
            return true;
        }

    };
}

Y_UNIT_TEST_SUITE(ParserSuite) {
    Y_UNIT_TEST(MultipartTest) {
        const TString content = R"(--uuid:f57dccb8-1d8c-423c-b000-eb69fdfe5b34
Content-Id: <rootpart*f57dccb8-1d8c-423c-b000-eb69fdfe5b34@example.jaxws.sun.com>
Content-Type: application/xop+xml;charset=utf-8;type="text/xml"
Content-Transfer-Encoding: binary

<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"><soap:Body><ns2:GetResponseResponse xmlns="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/basic/1.3" xmlns:ns2="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/1.3" xmlns:ns3="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/faults/1.3"><ns2:ResponseMessage><ns2:Response xmlns:ns2="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/1.3" Id="SIGNED_BY_SMEV"><ns2:OriginalMessageId>0af77560-6340-11ec-907d-7d6033a3053b</ns2:OriginalMessageId><ns2:SenderProvidedResponseData xmlns="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/basic/1.3" xmlns:ns2="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/1.3" xmlns:ns3="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/faults/1.3" xmlns:tns="urn://mincomsvyaz/esia/uprid/1.4.3" xmlns:fn="http://www.w3.org/2005/xpath-functions" xmlns:fo="http://www.w3.org/1999/XSL/Format" xmlns:ds="http://www.w3.org/2000/09/xmldsig#" xmlns:S="http://schemas.xmlsoap.org/soap/envelope/" Id="SIGNED_BY_CALLER"><ns2:MessageID>133b4229-6340-11ec-ba07-fa163e24a723</ns2:MessageID><ns2:To>eyJzaWQiOjk4OTgwNCwibWlkIjoiMGFmNzc1NjAtNjM0MC0xMWVjLTkwN2QtN2Q2MDMzYTMwNTNiIiwiZW9sIjowLCJzbGMiOiJtaW5jb21zdnlhel9lc2lhX3VwcmlkXzEuMi4wX0VTSUFEYXRhVmVyaWZ5UmVxdWVzdCIsIm1ubSI6Ijg0NDIwMiIsIm5zIjoidXJuOi8vbWluY29tc3Z5YXovZXNpYS91cHJpZC8xLjQuMyJ9</ns2:To><MessagePrimaryContent><tns:ESIADataVerifyResponse xmlns:tns="urn://mincomsvyaz/esia/uprid/1.4.3" xmlns:fn="http://www.w3.org/2005/xpath-functions" xmlns:fo="http://www.w3.org/1999/XSL/Format" xmlns:ns2="urn://mincomsvyaz/esia/commons/rg_sevices_types/1.4.3"><tns:status>VALID</tns:status><tns:requestId>1115335</tns:requestId></tns:ESIADataVerifyResponse></MessagePrimaryContent></ns2:SenderProvidedResponseData><st3:MessageMetadata xmlns:st3="urn://x-artefacts-smev-gov-ru/services/message-exchange/types/1.3"><st3:MessageId>133b4229-6340-11ec-ba07-fa163e24a723</st3:MessageId><st3:MessageType>RESPONSE</st3:MessageType><st3:Sender><st3:Mnemonic>emu</st3:Mnemonic><st3:HumanReadableName>emu</st3:HumanReadableName></st3:Sender><st3:SendingTimestamp>2021-12-22T18:58:59.425+03:00</st3:SendingTimestamp><st3:Recipient><st3:Mnemonic>844202</st3:Mnemonic><st3:HumanReadableName>ЕИШ 3.0 АО "Яндекс Банк"</st3:HumanReadableName></st3:Recipient><st3:DeliveryTimestamp>2021-12-27T17:16:05.141+03:00</st3:DeliveryTimestamp><st3:Status>messageIsDelivered</st3:Status></st3:MessageMetadata></ns2:Response><ns2:SMEVSignature><ds:Signature xmlns:ds="http://www.w3.org/2000/09/xmldsig#"><ds:SignedInfo><ds:CanonicalizationMethod Algorithm="http://www.w3.org/2001/10/xml-exc-c14n#"/><ds:SignatureMethod Algorithm="urn:ietf:params:xml:ns:cpxmlsec:algorithms:gostr34102012-gostr34112012-256"/><ds:Reference URI="#SIGNED_BY_SMEV"><ds:Transforms><ds:Transform Algorithm="http://www.w3.org/2001/10/xml-exc-c14n#"/><ds:Transform Algorithm="urn://smev-gov-ru/xmldsig/transform"/></ds:Transforms><ds:DigestMethod Algorithm="urn:ietf:params:xml:ns:cpxmlsec:algorithms:gostr34112012-256"/><ds:DigestValue>nfJ2vH/u1V88rOSXiWcvbMJBzeOsX3Uqaf+HTWpzB2M=</ds:DigestValue></ds:Reference></ds:SignedInfo><ds:SignatureValue>vS9kWpndDGbc/H6BUUVzG8sJhMQXqyNRrfTRG1sL9ZeKT3x3wb4WGZ9N+84ir4/rke3yB+hptI23Mf75ZQDR4A==</ds:SignatureValue><ds:KeyInfo><ds:X509Data><ds:X509Certificate>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</ds:X509Certificate></ds:X509Data></ds:KeyInfo></ds:Signature></ns2:SMEVSignature></ns2:ResponseMessage></ns2:GetResponseResponse></soap:Body></soap:Envelope>
--uuid:f57dccb8-1d8c-423c-b000-eb69fdfe5b34--
)";
        THttpHeaders headers;
        headers.AddHeader("Server", "nginx");
        headers.AddHeader("Date", "Mon, 27 Dec 2021 14:16:05 GMT");
        headers.AddHeader("Content-Type", "multipart/related; start*0=\"<rootpart*f57dccb8-1d8c-423c-b000-eb69fdfe5b34@example.jaxws\"; start*1=\".sun.com>\"; type=\"application/xop+xml\"; boundary=\"uuid:f57dccb8-1d8c-423c-b000-eb69fdfe5b34\"; start-info=\"text/xml\";charset=UTF-8");
        headers.AddHeader("Content-Length", "6765");
        headers.AddHeader("Connection", "keep-alive");
        headers.AddHeader("Keep-Alive", "timeout=60");
        TCallback ctxt;
        NExternalAPI::TParser parser(content, headers, ctxt);
        UNIT_ASSERT(parser.Parse());
    }
};
