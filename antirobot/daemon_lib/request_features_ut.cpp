#include "cgi_params.h"
#include "cookie_names.h"
#include "entity_dictionary.h"
#include "fullreq_info.h"
#include "request_features.h"
#include "rps_filter.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <antirobot/lib/keyring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/string/printf.h>

namespace NAntiRobot {

    class TTestRequestFeaturesParams : public TTestBase {
    public:
        void SetUp() override {
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(
                "<Daemon>\n"
                "CaptchaApiHost = ::\n"
                "CaptchaApiHost = ::\n"
                "CbbApiHost = ::\n"
                "CbbApiHost = ::\n"
                "FormulasDir = .\n"
                "FormulasDir = .\n"
                "</Daemon>\n"
            );
            TJsonConfigGenerator jsonConf;
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());


            const TString testKey = "102c46d700bed5c69ed20b7473886468";
            TStringInput keys(testKey);
            TKeyRing::SetInstance(TKeyRing(keys));
        }
    };

    Y_UNIT_TEST_SUITE_IMPL(TTestRequestFeatures, TTestRequestFeaturesParams) {

        Y_UNIT_TEST(TestUsual) {
            TString req =
                "GET /yandsearch?text=hello&lr=123&numdoc=25&p=3 HTTP/1.1\r\n"
                "Accept: image/gif, image/jpeg, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                "Referer: http://yAndeX.kz/yandsearch?text=%D0%B8%D0%BC%D1%8F+%D0%B2%D1%81%D0%B5%D1%85+%D0%B0%D0%BA%D1%82%D0%B5%D1%80%D0%BE%D0%B2+%D0%B8%D0%B7+%D1%81%D0%B5%D1%80%D0%B8%D0%B0%D0%BB%D0%B0+%D0%BF%D1%80%D0%BE%D0%BA%D0%BB%D1%8F%D1%82%D1%8B%D0%B9+%D1%80%D0%B0%D0%B9&lr=63\r\n"
                "Accept-Language: ru\r\n"
                "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; GTB6.5; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; InfoPath.2)\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "Host: yandex.kz\r\n"
                "Connection: Keep-Alive\r\n"
                "X-Yandex-ICookie: i_cookie\r\n"
                "Cookie: yandexuid=4151958451269067436; yabs-frequency=/3/rUCLPOm38NES56MC0o5kzXLbZ0CXjsuKPOm3OdVf5MMC0o4YymjbV0Ey//f0BZ01i3W0600O01X0E50uG10W00/fGBZ01m4W0600O010m600G420e410O01X0820W00; fuid01=4ba78514086b657f.DQ7lG1s0WWb5_1mEyZ1hHnsaBvUYivoP7BXtRGy2_ok6ABwwBd6FFsxJc0SMMIn-b81U67nexuq_TgVdyOJhabip6wPer6v8WbItnB2SyUtBlTAiCIZ9q_4muAYVHOO0; spravka=dD0xMjgwOTE5Njc1O2k9ODEuMTguMTE1LjE0Mjt1PTEyODA5MTk2NzUwMDA4NzA2NTM7aD02NTc5MWJhNjIzMDY4YTAwMWExMDQ4ZTJlNmFlNTIyYg%3D%3D\r\n"
                "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                "X-Forwarded-For-Y: 81.18.115.137\r\n"
                "X-Source-Port-Y: 18671\r\n"
                "X-Start-Time: 1280969722810839\r\n"
                "\r\n";

            TAutoPtr<TRequestFeatures> rf = MakeReqFeatures(req);

            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), StrToIp("81.18.115.137"));
            UNIT_ASSERT_EQUAL(rf->Request->SpravkaAddr, TAddr("81.18.115.142"));
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Ns, TUid::SPRAVKA);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Id, 1280919675000870653ULL);
            UNIT_ASSERT(rf->Request->HasValidSpravka);
            UNIT_ASSERT(!rf->Request->HasValidFuid);
            UNIT_ASSERT_EQUAL(rf->Request->ICookie, "i_cookie");

            UNIT_ASSERT_STRINGS_EQUAL(rf->ReqText, "hello");

            UNIT_ASSERT_EQUAL(rf->ReqType, REQ_YANDSEARCH);

            UNIT_ASSERT_EQUAL(rf->HostType, HOST_WEB);

            UNIT_ASSERT_EQUAL(rf->Request->ArrivalTime, TInstant::MicroSeconds(1280969722810839ULL));

            UNIT_ASSERT(rf->Request->IsSearch);
            UNIT_ASSERT(!rf->IsYandsearchDzen());

            UNIT_ASSERT(!rf->XffExists);
            UNIT_ASSERT(!rf->NoCookies);
            UNIT_ASSERT(!rf->NoReferer);
            UNIT_ASSERT(rf->RefererFromYandex);
            UNIT_ASSERT(!rf->IsBadUserAgent);
            UNIT_ASSERT(!rf->IsTimeSort);
            UNIT_ASSERT_EQUAL(rf->NumDocs, 25);
            UNIT_ASSERT(!rf->IsBadProtocol);
            UNIT_ASSERT(rf->IsConnectionKeepAlive);
            UNIT_ASSERT(!rf->AdvancedSearch);

            UNIT_ASSERT(!rf->LCookieExists);

            UNIT_ASSERT_EQUAL(rf->PageNum, 3);

            UNIT_ASSERT(rf->CgiParamPresent[0]);
            UNIT_ASSERT(rf->CgiParamPresent[1]);
            UNIT_ASSERT(!rf->CgiParamPresent[2]);
            UNIT_ASSERT(!rf->CgiParamPresent[3]);
            UNIT_ASSERT(rf->CgiParamPresent[5]);

            UNIT_ASSERT(!rf->CookiePresent[0]);
            UNIT_ASSERT(!rf->CookiePresent[1]);
            UNIT_ASSERT(rf->CookiePresent[6]);
            UNIT_ASSERT(!rf->CookiePresent[7]);
            UNIT_ASSERT(rf->CookiePresent[8]);
            UNIT_ASSERT(rf->CookiePresent[13]);
            UNIT_ASSERT(rf->CookiePresent[17]);

            UNIT_ASSERT(rf->HttpHeaderPresent[0]);
            UNIT_ASSERT(!rf->HttpHeaderPresent[1]);
            UNIT_ASSERT(rf->HttpHeaderPresent[2]);
            UNIT_ASSERT(rf->HttpHeaderPresent[3]);
            UNIT_ASSERT(!rf->HttpHeaderPresent[4]);

            UNIT_ASSERT(!rf->HaveUnknownHeaders);
            UNIT_ASSERT_EQUAL(rf->HeadersCount, 8);
        }

        Y_UNIT_TEST(TestUnusual1) {
            // fabricate config
            const char* cfg = "<Daemon>\n\
                                   AuthorizeByFuid = 1\n\
                                   CaptchaApiHost = ::\n\
                                   CbbApiHost = ::\n\
                               </Daemon>";
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(cfg);

            TString req =
                "GET /search?text=hello&numdoc=25&p=3&randomtext= HTTP/1.0\r\n"
                "Host: images.yandex.ru\r\n"
                "Cookie: fuid01=4ba78514086b657f.DQ7lG1s0WWb5_1mEyZ1hHnsaBvUYivoP7BXtRGy2_ok6ABwwBd6FFsxJc0SMMIn-b81U67nexuq_TgVdyOJhabip6wPer6v8WbItnB2SyUtBlTAiCIZ9q_4muAYVHOO0; L=123456; YX_SEARCHPREFS=numdoc:35\r\n"
                "Some-Unknown-Header: blahblah\r\n"
                "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                "X-Forwarded-For-Y: 81.18.115.137\r\n"
                "X-Start-Time: 1280969722810839\r\n"
                "\r\n";

            TAutoPtr<TRequestFeatures> rf = MakeReqFeatures(req);

            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), StrToIp("81.18.115.137"));
            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), rf->Request->UserAddr.AsIp());
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Ns, TUid::IP);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Id, 1360163721ULL);
            UNIT_ASSERT(!rf->Request->HasValidSpravka);
            UNIT_ASSERT(!rf->Request->HasValidFuid);
            UNIT_ASSERT(rf->Request->ICookie.empty());

            UNIT_ASSERT_STRINGS_EQUAL(rf->ReqText, "hello");

            UNIT_ASSERT_EQUAL(rf->ReqType, REQ_YANDSEARCH);
            UNIT_ASSERT_EQUAL(rf->HostType, HOST_IMAGES);

            UNIT_ASSERT_EQUAL(rf->Request->ArrivalTime, TInstant::MicroSeconds(1280969722810839ULL));

            UNIT_ASSERT(rf->Request->IsSearch);
            UNIT_ASSERT(!rf->IsYandsearchDzen());

            UNIT_ASSERT(!rf->XffExists);
            UNIT_ASSERT(!rf->NoCookies);
            UNIT_ASSERT(rf->NoReferer);
            UNIT_ASSERT(!rf->RefererFromYandex);
            UNIT_ASSERT(rf->IsBadUserAgent);
            UNIT_ASSERT(!rf->IsTimeSort);
            UNIT_ASSERT_EQUAL(rf->NumDocs, 25);
            UNIT_ASSERT(rf->IsBadProtocol);
            UNIT_ASSERT(!rf->IsConnectionKeepAlive);
            UNIT_ASSERT(!rf->AdvancedSearch);

            UNIT_ASSERT(rf->LCookieExists);

            UNIT_ASSERT_EQUAL(rf->PageNum, 3);

            UNIT_ASSERT(rf->CgiParamPresent[0]);
            UNIT_ASSERT(!rf->CgiParamPresent[1]);
            UNIT_ASSERT(!rf->CgiParamPresent[2]);
            UNIT_ASSERT(!rf->CgiParamPresent[3]);
            UNIT_ASSERT(rf->CgiParamPresent[5]);

            UNIT_ASSERT(rf->CookiePresent[1]);
            UNIT_ASSERT(!rf->CookiePresent[6]);
            UNIT_ASSERT(!rf->CookiePresent[7]);
            UNIT_ASSERT(rf->CookiePresent[16]);
            UNIT_ASSERT(rf->CookiePresent[17]);

            UNIT_ASSERT(!rf->HttpHeaderPresent[0]);
            UNIT_ASSERT(!rf->HttpHeaderPresent[1]);
            UNIT_ASSERT(rf->HttpHeaderPresent[7]);
            UNIT_ASSERT(rf->HttpHeaderPresent[14]);

            UNIT_ASSERT(rf->HaveUnknownHeaders);
            UNIT_ASSERT_EQUAL(rf->HeadersCount, 3);
        }

        Y_UNIT_TEST(TestUnusual2) {
            // fabricate config
            TString req =
                "GET /search?text=hello&p=3&lr=123&randomtext=&how=tm HTTP/1.0\r\n"
                "Accept-Language: ru\r\n"
                "Referer: http://ya.ru\n"
                "Host: yandex.kz\r\n"
                "Cookie: L=123456; YX_SEARCHPREFS=numdoc:35\r\n"
                "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                "X-Forwarded-For-Y: 81.18.115.137\r\n"
                "X-Start-Time: 1280969722810839\r\n"
                "\r\n";

            TAutoPtr<TRequestFeatures> rf = MakeReqFeatures(req);

            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), StrToIp("81.18.115.137"));
            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), rf->Request->UserAddr.AsIp());
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Ns, TUid::IP);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Id, rf->Request->UserAddr.AsIp());
            UNIT_ASSERT(!rf->Request->HasValidSpravka);
            UNIT_ASSERT(!rf->Request->HasValidFuid);

            UNIT_ASSERT_STRINGS_EQUAL(rf->ReqText, "hello");

            UNIT_ASSERT_EQUAL(rf->ReqType, REQ_YANDSEARCH);
            UNIT_ASSERT_EQUAL(rf->HostType, HOST_WEB);

            UNIT_ASSERT_EQUAL(rf->Request->ArrivalTime, TInstant::MicroSeconds(1280969722810839ULL));

            UNIT_ASSERT(rf->Request->IsSearch);
            UNIT_ASSERT(rf->IsYandsearchDzen());

            UNIT_ASSERT(!rf->XffExists);
            UNIT_ASSERT(!rf->NoCookies);
            UNIT_ASSERT(!rf->NoReferer);
            UNIT_ASSERT(rf->RefererFromYandex);
            UNIT_ASSERT(rf->IsBadUserAgent);
            UNIT_ASSERT(rf->IsTimeSort);
            UNIT_ASSERT_EQUAL(rf->NumDocs, 35);
            UNIT_ASSERT(rf->IsBadProtocol);
            UNIT_ASSERT(!rf->IsConnectionKeepAlive);
            UNIT_ASSERT(!rf->AdvancedSearch);

            UNIT_ASSERT(rf->LCookieExists);

            UNIT_ASSERT_EQUAL(rf->PageNum, 3);

            UNIT_ASSERT(rf->CgiParamPresent[0]);
            UNIT_ASSERT(rf->CgiParamPresent[1]);
            UNIT_ASSERT(!rf->CgiParamPresent[2]);
            UNIT_ASSERT(!rf->CgiParamPresent[3]);
            UNIT_ASSERT(rf->CgiParamPresent[5]);

            UNIT_ASSERT(rf->CookiePresent[1]);
            UNIT_ASSERT(!rf->CookiePresent[6]);
            UNIT_ASSERT(rf->CookiePresent[16]);
            UNIT_ASSERT(!rf->CookiePresent[17]);

            UNIT_ASSERT(!rf->HttpHeaderPresent[0]);
            UNIT_ASSERT(rf->HttpHeaderPresent[3]);
            UNIT_ASSERT(rf->HttpHeaderPresent[7]);
            UNIT_ASSERT(rf->HttpHeaderPresent[14]);
            UNIT_ASSERT(rf->HttpHeaderPresent[24]);
            UNIT_ASSERT(!rf->HttpHeaderPresent[25]);

            UNIT_ASSERT(!rf->HaveUnknownHeaders);
            UNIT_ASSERT_EQUAL(rf->HeadersCount, 4);
        }

        Y_UNIT_TEST(TestNoLr) {
            TString req =
                "GET /yandsearch?text=hello&numdoc=25&p=3 HTTP/1.1\r\n"
                "Accept: image/gif, image/jpeg, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                "Referer: http://yAndeX.kz/yandsearch?text=%D0%B8%D0%BC%D1%8F+%D0%B2%D1%81%D0%B5%D1%85+%D0%B0%D0%BA%D1%82%D0%B5%D1%80%D0%BE%D0%B2+%D0%B8%D0%B7+%D1%81%D0%B5%D1%80%D0%B8%D0%B0%D0%BB%D0%B0+%D0%BF%D1%80%D0%BE%D0%BA%D0%BB%D1%8F%D1%82%D1%8B%D0%B9+%D1%80%D0%B0%D0%B9&lr=63\r\n"
                "Accept-Language: ru\r\n"
                "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; GTB6.5; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; InfoPath.2)\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "Host: yandex.kz\r\n"
                "Connection: Keep-Alive\r\n"
                "Cookie: yandexuid=4151958451269067436; yabs-frequency=/3/rUCLPOm38NES56MC0o5kzXLbZ0CXjsuKPOm3OdVf5MMC0o4YymjbV0Ey//f0BZ01i3W0600O01X0E50uG10W00/fGBZ01m4W0600O010m600G420e410O01X0820W00; fuid01=4ba78514086b657f.DQ7lG1s0WWb5_1mEyZ1hHnsaBvUYivoP7BXtRGy2_ok6ABwwBd6FFsxJc0SMMIn-b81U67nexuq_TgVdyOJhabip6wPer6v8WbItnB2SyUtBlTAiCIZ9q_4muAYVHOO0; spravka=dD0xMjgwOTE5Njc1O2k9ODEuMTguMTE1LjE0Mjt1PTEyODA5MTk2NzUwMDA4NzA2NTM7aD02NTc5MWJhNjIzMDY4YTAwMWExMDQ4ZTJlNmFlNTIyYg%3D%3D\r\n"
                "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                "X-Forwarded-For-Y: 81.18.115.137\r\n"
                "X-Source-Port-Y: 18671\r\n"
                "X-Start-Time: 1280969722810839\r\n"
                "\r\n";

            TAutoPtr<TRequestFeatures> rf = MakeReqFeatures(req);

            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), StrToIp("81.18.115.137"));
            UNIT_ASSERT_EQUAL(rf->Request->SpravkaAddr, TAddr("81.18.115.142"));
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Ns, TUid::SPRAVKA);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Id, 1280919675000870653ULL);
            UNIT_ASSERT(rf->Request->HasValidSpravka);
            UNIT_ASSERT(!rf->Request->HasValidFuid);

            UNIT_ASSERT_STRINGS_EQUAL(rf->ReqText, "hello");

            UNIT_ASSERT_EQUAL(rf->ReqType, REQ_YANDSEARCH);
            UNIT_ASSERT_EQUAL(rf->HostType, HOST_WEB);

            UNIT_ASSERT_EQUAL(rf->Request->ArrivalTime, TInstant::MicroSeconds(1280969722810839ULL));

            UNIT_ASSERT(rf->Request->IsSearch);

            UNIT_ASSERT(!rf->XffExists);
            UNIT_ASSERT(!rf->NoCookies);
            UNIT_ASSERT(!rf->NoReferer);
            UNIT_ASSERT(rf->RefererFromYandex);
            UNIT_ASSERT(!rf->IsBadUserAgent);
            UNIT_ASSERT(!rf->IsTimeSort);
            UNIT_ASSERT_EQUAL(rf->NumDocs, 25);
            UNIT_ASSERT(!rf->IsBadProtocol);
            UNIT_ASSERT(rf->IsConnectionKeepAlive);
            UNIT_ASSERT(!rf->AdvancedSearch);

            UNIT_ASSERT(!rf->LCookieExists);

            UNIT_ASSERT_EQUAL(rf->PageNum, 3);

            UNIT_ASSERT(rf->Request->CanShowCaptcha());
        }

        Y_UNIT_TEST(TestXmlsearchPartnerPostSimple) {
            TString req =
                "POST /xmlsearch?showmecaptcha=yes&user=partner&key=xml-key HTTP/1.1\r\n"
                "Host: xmlsearch.yandex.ru\r\n"
                "Accept-Encoding: identity\r\n"
                "Content-Length: 77\r\n"
                "Accept-Charset: utf-8\r\n"
                "Accept-Language: ru,en-us\r\n"
                "X-Real-Ip: 84.201.167.197\r\n"
                "Referer: http://blah-blah.ru:23433/q?request=123\r\n"
                "Content-Type: Application/xml\r\n"
                "Connection: close\r\n"
                "Accept: application/xhtml+xml,application/xml\r\n"
                "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:6.0.2) Gecko/20100101 Firefox/6.0.2\r\n"
                "X-Req-Id: 1315558358047850-8369549177296019777\r\n"
                "X-Forwarded-For-Y: 93.158.131.173\r\n"
                "X-Source-Port-Y: 59148\r\n"
                "X-Start-Time: 1315558358047850\r\n"
                "\r\n"
                "<?xml version='1.0' encoding='UTF-8'?>\r\n"
                "<request><query>123</query></request>\r\n";

            TAutoPtr<TRequestFeatures> rf = MakeReqFeatures(req);

            UNIT_ASSERT_VALUES_EQUAL(rf->ReqType, REQ_YANDSEARCH);
            UNIT_ASSERT_EQUAL(rf->HostType, HOST_WEB);

            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), StrToIp("84.201.167.197"));
            UNIT_ASSERT_EQUAL(rf->Request->PartnerAddr.AsIp(), StrToIp("93.158.131.173"));
            UNIT_ASSERT_EQUAL(rf->Request->SpravkaAddr, rf->Request->UserAddr);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Ns, TUid::IP);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Id, rf->Request->UserAddr.AsIp());

            UNIT_ASSERT_EQUAL(rf->ReqText, "123");

            UNIT_ASSERT(!rf->Request->HasValidSpravka);
            UNIT_ASSERT(rf->Request->HasValidFuid);

            UNIT_ASSERT_EQUAL(rf->Request->ArrivalTime, TInstant::MicroSeconds(1315558358047850ULL));

            UNIT_ASSERT(rf->Request->IsSearch);

            UNIT_ASSERT(!rf->XffExists);
            UNIT_ASSERT(!rf->NoCookies);
            UNIT_ASSERT(!rf->NoReferer);
            UNIT_ASSERT(rf->RefererFromYandex);
            UNIT_ASSERT(!rf->IsBadUserAgent);
            UNIT_ASSERT(!rf->IsTimeSort);
            UNIT_ASSERT_EQUAL(rf->NumDocs, 10);
            UNIT_ASSERT(!rf->IsBadProtocol);
            UNIT_ASSERT(rf->IsConnectionKeepAlive);
            UNIT_ASSERT(!rf->AdvancedSearch);

            UNIT_ASSERT(!rf->LCookieExists);

            UNIT_ASSERT_EQUAL(rf->PageNum, 0);

            UNIT_ASSERT(rf->Request->CanShowCaptcha());
        }

        Y_UNIT_TEST(TestXmlsearchPartnerPostWithNumdoc) {
            TString req =
                "POST /xmlsearch?showmecaptcha=yes&user=partner&key=xml-key HTTP/1.1\r\n"
                "Accept-Encoding: identity\r\n"
                "Content-Length: 295\r\n"
                "Accept-Charset: utf-8\r\n"
                "Host: xmlsearch.yandex.ru\r\n"
                "Accept-Language: ru,en-us\r\n"
                "X-Real-Ip: 84.201.167.197\r\n"
                "Referer: http://blah-blah.ru:23433/q?request=123\r\n"
                "Content-Type: Application/xml\r\n"
                "Connection: close\r\n"
                "Accept: application/xhtml+xml,application/xml\r\n"
                "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:6.0.2) Gecko/20100101 Firefox/6.0.2\r\n"
                "X-Req-Id: 1315558358047850-8369549177296019777\r\n"
                "X-Forwarded-For-Y: 93.158.131.173\r\n"
                "X-Source-Port-Y: 59148\r\n"
                "X-Start-Time: 1315558358047850\r\n"
                "\r\n"
                "<?xml version='1.0' encoding='UTF-8'?>\r\n"
                "<request><query>1234</query><page>2</page>\r\n"
                "<groupings><groupby attr='ih' mode='deep' groups-on-page='10' docs-in-group='1' curcateg='2'/>\r\n"
                "<groupby attr='d' mode='deep' groups-on-page='50' docs-in-group='2' depth='1' killdup='yes'/></groupings></request>\r\n";

            TAutoPtr<TRequestFeatures> rf = MakeReqFeatures(req);

            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), StrToIp("84.201.167.197"));
            UNIT_ASSERT_EQUAL(rf->Request->SpravkaAddr, rf->Request->UserAddr);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Ns, TUid::IP);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Id, rf->Request->UserAddr.AsIp());
            UNIT_ASSERT(!rf->Request->HasValidSpravka);
            UNIT_ASSERT(rf->Request->HasValidFuid);

            UNIT_ASSERT_EQUAL(rf->ReqText, "1234");

            UNIT_ASSERT_EQUAL(rf->ReqType, REQ_YANDSEARCH);
            UNIT_ASSERT_EQUAL(rf->HostType, HOST_WEB);

            UNIT_ASSERT_EQUAL(rf->Request->ArrivalTime, TInstant::MicroSeconds(1315558358047850ULL));

            UNIT_ASSERT(rf->Request->IsSearch);

            UNIT_ASSERT(!rf->XffExists);
            UNIT_ASSERT(!rf->NoCookies);
            UNIT_ASSERT(!rf->NoReferer);
            UNIT_ASSERT(rf->RefererFromYandex);
            UNIT_ASSERT(!rf->IsBadUserAgent);
            UNIT_ASSERT(!rf->IsTimeSort);
            UNIT_ASSERT_EQUAL(rf->NumDocs, 50);
            UNIT_ASSERT(!rf->IsBadProtocol);
            UNIT_ASSERT(rf->IsConnectionKeepAlive);
            UNIT_ASSERT(!rf->AdvancedSearch);

            UNIT_ASSERT(!rf->LCookieExists);

            UNIT_ASSERT_EQUAL(rf->PageNum, 2);

            UNIT_ASSERT(rf->Request->CanShowCaptcha());
        }

        Y_UNIT_TEST(TestXmlsearchPartnerGetSimple) {
            TString req =
                "GET /xmlsearch?showmecaptcha=yes&user=partner&key=xml-key&query=12345 HTTP/1.1\r\n"
                "Accept-Encoding: identity\r\n"
                "Accept-Charset: utf-8\r\n"
                "Host: xmlsearch.yandex.ru\r\n"
                "Accept-Language: ru,en-us\r\n"
                "X-Real-Ip: 84.201.167.197\r\n"
                "Referer: http://blah-blah.ru:23433/q?request=123\r\n"
                "Content-Type: Application/xml\r\n"
                "Connection: close\r\n"
                "Accept: application/xhtml+xml,application/xml\r\n"
                "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:6.0.2) Gecko/20100101 Firefox/6.0.2\r\n"
                "X-Req-Id: 1315558358047850-8369549177296019777\r\n"
                "X-Forwarded-For-Y: 93.158.131.173\r\n"
                "X-Source-Port-Y: 59148\r\n"
                "X-Start-Time: 1315558358047850\r\n";

            TAutoPtr<TRequestFeatures> rf = MakeReqFeatures(req);

            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), StrToIp("84.201.167.197"));
            UNIT_ASSERT_EQUAL(rf->Request->SpravkaAddr, rf->Request->UserAddr);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Ns, TUid::IP);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Id, rf->Request->UserAddr.AsIp());
            UNIT_ASSERT(!rf->Request->HasValidSpravka);
            UNIT_ASSERT(rf->Request->HasValidFuid);

            UNIT_ASSERT_EQUAL(rf->ReqText, "12345");

            UNIT_ASSERT_EQUAL(rf->ReqType, REQ_YANDSEARCH);
            UNIT_ASSERT_EQUAL(rf->HostType, HOST_WEB);

            UNIT_ASSERT_EQUAL(rf->Request->ArrivalTime, TInstant::MicroSeconds(1315558358047850ULL));

            UNIT_ASSERT(rf->Request->IsSearch);

            UNIT_ASSERT(!rf->XffExists);
            UNIT_ASSERT(!rf->NoCookies);
            UNIT_ASSERT(!rf->NoReferer);
            UNIT_ASSERT(rf->RefererFromYandex);
            UNIT_ASSERT(!rf->IsBadUserAgent);
            UNIT_ASSERT(!rf->IsTimeSort);
            UNIT_ASSERT_EQUAL(rf->NumDocs, 10);
            UNIT_ASSERT(!rf->IsBadProtocol);
            UNIT_ASSERT(rf->IsConnectionKeepAlive);
            UNIT_ASSERT(!rf->AdvancedSearch);

            UNIT_ASSERT(!rf->LCookieExists);

            UNIT_ASSERT_EQUAL(rf->PageNum, 0);

            UNIT_ASSERT(rf->Request->CanShowCaptcha());
        }

        Y_UNIT_TEST(TestXmlsearchPartnerGetWithNumdoc) {
            TString req =
                "GET /xmlsearch?showmecaptcha=yes&user=partner&key=xml-key&query=123456"
                    "&page=4&sortby=rlv&max-passage-length=400&max-title-length=75&max-headline-length=300&max-text-length=500"
                    "&reqid=request-id&nocache="
                    "&groupby=attr%3Dih.mode%3Ddeep.groups-on-page%3D10.docs-in-group%3D1.curcateg%3D2."
                    "&groupby=attr%3Dd.mode%3Ddeep.groups-on-page%3D40.docs-in-group%3D2.depth%3D1.killdup%3Dyes. HTTP/1.1\r\n"
                "Accept-Encoding: identity\r\n"
                "Accept-Charset: utf-8\r\n"
                "Host: xmlsearch.yandex.ru\r\n"
                "Accept-Language: ru,en-us\r\n"
                "X-Real-Ip: 84.201.167.197\r\n"
                "Referer: http://blah-blah.ru:23433/q?request=123\r\n"
                "Content-Type: Application/xml\r\n"
                "Connection: close\r\n"
                "Accept: application/xhtml+xml,application/xml\r\n"
                "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:6.0.2) Gecko/20100101 Firefox/6.0.2\r\n"
                "X-Req-Id: 1315558358047850-8369549177296019777\r\n"
                "X-Forwarded-For-Y: 93.158.131.173\r\n"
                "X-Source-Port-Y: 59148\r\n"
                "X-Start-Time: 1315558358047850\r\n";

            TAutoPtr<TRequestFeatures> rf = MakeReqFeatures(req);

            UNIT_ASSERT_EQUAL(rf->Request->UserAddr.AsIp(), StrToIp("84.201.167.197"));
            UNIT_ASSERT_EQUAL(rf->Request->SpravkaAddr, rf->Request->UserAddr);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Ns, TUid::IP);
            UNIT_ASSERT_EQUAL(rf->Request->Uid.Id, rf->Request->UserAddr.AsIp());
            UNIT_ASSERT(!rf->Request->HasValidSpravka);
            UNIT_ASSERT(rf->Request->HasValidFuid);

            UNIT_ASSERT_EQUAL(rf->ReqText, "123456");

            UNIT_ASSERT_EQUAL(rf->ReqType, REQ_YANDSEARCH);
            UNIT_ASSERT_EQUAL(rf->HostType, HOST_WEB);

            UNIT_ASSERT_EQUAL(rf->Request->ArrivalTime, TInstant::MicroSeconds(1315558358047850ULL));

            UNIT_ASSERT(rf->Request->IsSearch);

            UNIT_ASSERT(!rf->XffExists);
            UNIT_ASSERT(!rf->NoCookies);
            UNIT_ASSERT(!rf->NoReferer);
            UNIT_ASSERT(rf->RefererFromYandex);
            UNIT_ASSERT(!rf->IsBadUserAgent);
            UNIT_ASSERT(!rf->IsTimeSort);
            UNIT_ASSERT_EQUAL(rf->NumDocs, 40);
            UNIT_ASSERT(!rf->IsBadProtocol);
            UNIT_ASSERT(rf->IsConnectionKeepAlive);
            UNIT_ASSERT(!rf->AdvancedSearch);

            UNIT_ASSERT(!rf->LCookieExists);

            UNIT_ASSERT_EQUAL(rf->PageNum, 4);

            UNIT_ASSERT(rf->Request->CanShowCaptcha());
        }

        Y_UNIT_TEST(TestCookiesFeatures) {
            {
                TString req =
                    "GET /film/1443803?qwe=1 HTTP/1.1\r\n"
                    "User-Agent: Test\r\n"
                    "Host: kinopoisk.ru\r\n"
                    "Accept: image/gif, image/gif, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                    "Accept-Language: ru\r\n"
                    "Accept-Encoding: gzip, deflate\r\n"
                    "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                    "X-Forwarded-For-Y: 81.18.116.137\r\n"
                    "X-Start-Time: 1280969722810839\r\n"
                    "Cookie: amcuid=1; currentRegionId=2; cycada=3; local-offers-first=4; lr=5; market_ys=6; onstock=7; yandex_help=8; cmp-merge=9; computer=10; head_baner=11; utm_source=12\r\n"
                    "\r\n";
                const auto rf = MakeReqFeatures(req);

                UNIT_ASSERT(rf->HasCookieAmcuid);
                UNIT_ASSERT(rf->HasCookieCurrentRegionId);
                UNIT_ASSERT(rf->HasCookieCycada);
                UNIT_ASSERT(rf->HasCookieLocalOffersFirst);
                UNIT_ASSERT(rf->HasCookieLr);
                UNIT_ASSERT(rf->HasCookieMarketYs);
                UNIT_ASSERT(rf->HasCookieOnstock);
                UNIT_ASSERT(rf->HasCookieYandexHelp);
                UNIT_ASSERT(rf->HasCookieCmpMerge);
                UNIT_ASSERT(rf->HasCookieComputer);
                UNIT_ASSERT(rf->HasCookieHeadBaner);
                UNIT_ASSERT(rf->HasCookieUtmSource);
            }
            {
                TString req =
                    "GET /film/1443803?qwe=1 HTTP/1.1\r\n"
                    "User-Agent: Test\r\n"
                    "Host: kinopoisk.ru\r\n"
                    "Accept: image/gif, image/gif, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                    "Accept-Language: ru\r\n"
                    "Accept-Encoding: gzip, deflate\r\n"
                    "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                    "X-Forwarded-For-Y: 81.18.116.137\r\n"
                    "X-Start-Time: 1280969722810839\r\n"
                    "\r\n";
                const auto rf = MakeReqFeatures(req);

                UNIT_ASSERT(!rf->HasCookieAmcuid);
                UNIT_ASSERT(!rf->HasCookieCurrentRegionId);
                UNIT_ASSERT(!rf->HasCookieCycada);
                UNIT_ASSERT(!rf->HasCookieLocalOffersFirst);
                UNIT_ASSERT(!rf->HasCookieLr);
                UNIT_ASSERT(!rf->HasCookieMarketYs);
                UNIT_ASSERT(!rf->HasCookieOnstock);
                UNIT_ASSERT(!rf->HasCookieYandexHelp);
                UNIT_ASSERT(!rf->HasCookieCmpMerge);
                UNIT_ASSERT(!rf->HasCookieComputer);
                UNIT_ASSERT(!rf->HasCookieHeadBaner);
                UNIT_ASSERT(!rf->HasCookieUtmSource);
            }
        }

        Y_UNIT_TEST(TestCookieAge) {
            {
                TString req =
                    "GET /film/1443803?qwe=1 HTTP/1.1\r\n"
                    "User-Agent: Test\r\n"
                    "Host: kinopoisk.ru\r\n"
                    "Accept: image/gif, image/gif, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                    "Accept-Language: ru\r\n"
                    "Accept-Encoding: gzip, deflate\r\n"
                    "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                    "X-Forwarded-For-Y: 81.18.116.137\r\n"
                    "X-Start-Time: 2281033839010338\r\n"
                    "X-Yandex-ICookie: 128096972281033838\r\n"
                    "Cookie: amcuid=1; currentRegionId=2; cycada=3; local-offers-first=4; lr=5; market_ys=6; onstock=7; yandex_help=8; cmp-merge=9; computer=10; head_baner=11; utm_source=12\r\n"
                    "\r\n";
                const auto rf = MakeReqFeatures(req);

                UNIT_ASSERT(rf->CookieYoungerMinute);
                UNIT_ASSERT(rf->CookieYoungerHour);
                UNIT_ASSERT(rf->CookieYoungerDay);
                UNIT_ASSERT(rf->CookieYoungerWeek);
                UNIT_ASSERT(rf->CookieYoungerMonth);
                UNIT_ASSERT(rf->CookieYoungerThreeMonthes);
                UNIT_ASSERT(!rf->CookieOlderMonth);
                UNIT_ASSERT(!rf->CookieOlderThreeMonthes);
            }

            {
                TString req =
                    "GET /film/1443803?qwe=1 HTTP/1.1\r\n"
                    "User-Agent: Test\r\n"
                    "Host: kinopoisk.ru\r\n"
                    "Accept: image/gif, image/gif, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                    "Accept-Language: ru\r\n"
                    "Accept-Encoding: gzip, deflate\r\n"
                    "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                    "X-Forwarded-For-Y: 81.18.116.137\r\n"
                    "X-Start-Time: 2283625848010338\r\n"
                    "X-Yandex-ICookie: 128096972281033838\r\n"
                    "Cookie: amcuid=1; currentRegionId=2; cycada=3; local-offers-first=4; lr=5; market_ys=6; onstock=7; yandex_help=8; cmp-merge=9; computer=10; head_baner=11; utm_source=12\r\n"
                    "\r\n";
                const auto rf = MakeReqFeatures(req);

                UNIT_ASSERT(!rf->CookieYoungerMinute);
                UNIT_ASSERT(!rf->CookieYoungerHour);
                UNIT_ASSERT(!rf->CookieYoungerDay);
                UNIT_ASSERT(!rf->CookieYoungerWeek);
                UNIT_ASSERT(!rf->CookieYoungerMonth);
                UNIT_ASSERT(rf->CookieYoungerThreeMonthes);
                UNIT_ASSERT(rf->CookieOlderMonth);
                UNIT_ASSERT(!rf->CookieOlderThreeMonthes);
            }
        }

    };

    Y_UNIT_TEST_SUITE_IMPL(TTestCacherRequestFeatures, TTestRequestFeaturesParams) {

        Y_UNIT_TEST(TestUsual) {
            TString req =
                "GET /yandsearch?text=hello&lr=123&numdoc=25&p=3 HTTP/1.1\r\n"
                "Accept: image/gif, image/jpeg, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                "Referer: http://yAndeX.kz/yandsearch?text=%D0%B8%D0%BC%D1%8F+%D0%B2%D1%81%D0%B5%D1%85+%D0%B0%D0%BA%D1%82%D0%B5%D1%80%D0%BE%D0%B2+%D0%B8%D0%B7+%D1%81%D0%B5%D1%80%D0%B8%D0%B0%D0%BB%D0%B0+%D0%BF%D1%80%D0%BE%D0%BA%D0%BB%D1%8F%D1%82%D1%8B%D0%B9+%D1%80%D0%B0%D0%B9&lr=63\r\n"
                "Accept-Language: ru\r\n"
                "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; GTB6.5; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; InfoPath.2)\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "Host: yandex.kz\r\n"
                "Connection: Keep-Alive\r\n"
                "X-Yandex-ICookie: i_cookie\r\n"
                "Cookie: yandexuid=4151958451269067436; yabs-frequency=/3/rUCLPOm38NES56MC0o5kzXLbZ0CXjsuKPOm3OdVf5MMC0o4YymjbV0Ey//f0BZ01i3W0600O01X0E50uG10W00/fGBZ01m4W0600O010m600G420e410O01X0820W00; fuid01=4ba78514086b657f.DQ7lG1s0WWb5_1mEyZ1hHnsaBvUYivoP7BXtRGy2_ok6ABwwBd6FFsxJc0SMMIn-b81U67nexuq_TgVdyOJhabip6wPer6v8WbItnB2SyUtBlTAiCIZ9q_4muAYVHOO0; spravka=dD0xMjgwOTE5Njc1O2k9ODEuMTguMTE1LjE0Mjt1PTEyODA5MTk2NzUwMDA4NzA2NTM7aD02NTc5MWJhNjIzMDY4YTAwMWExMDQ4ZTJlNmFlNTIyYg%3D%3D\r\n"
                "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                "X-Forwarded-For-Y: 81.18.115.137\r\n"
                "X-Source-Port-Y: 18671\r\n"
                "X-Start-Time: 1280969722810839\r\n"
                "X-Yandex-Ja3: 771,49195-49196-52393-49199-49200-52392-49171-49172-156-157-47-53,65281-0-23-35-13-5-16-11-10,29-23-24,0\r\n"
                "\r\n";

            const TCacherRequestFeatures rf = MakeCacherReqFeatures(req);
            UNIT_ASSERT_EQUAL(rf.NumDocs, 25);
            UNIT_ASSERT_EQUAL(rf.PageNum, 3);
            UNIT_ASSERT_EQUAL(rf.HeadersCount, 8);

            UNIT_ASSERT(rf.RefererFromYandex);
            UNIT_ASSERT(!rf.IsBadProtocol);
            UNIT_ASSERT(!rf.IsBadUserAgent);
            UNIT_ASSERT(rf.IsConnectionKeepAlive);
            UNIT_ASSERT(!rf.HaveUnknownHeaders);

            UNIT_ASSERT(rf.CgiParamPresent[FindArrayIndex(CacherCgiParamName, "text"_sb)]);
            UNIT_ASSERT(rf.CgiParamPresent[FindArrayIndex(CacherCgiParamName, "lr"_sb)]);
            UNIT_ASSERT(!rf.CgiParamPresent[FindArrayIndex(CacherCgiParamName,"clid"_sb)]);
            UNIT_ASSERT(!rf.CgiParamPresent[FindArrayIndex(CacherCgiParamName,"tld"_sb)]);
            UNIT_ASSERT(!rf.CgiParamPresent[FindArrayIndex(CacherCgiParamName, "url"_sb)]);
            UNIT_ASSERT(!rf.CgiParamPresent[FindArrayIndex(CacherCgiParamName, "site"_sb)]);
            UNIT_ASSERT(!rf.CgiParamPresent[FindArrayIndex(CacherCgiParamName, "lang"_sb)]);

            UNIT_ASSERT(rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "Accept-Encoding"_sb)]);
            UNIT_ASSERT(rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "Accept-Language"_sb)]);
            UNIT_ASSERT(!rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName,"Authorization"_sb)]);
            UNIT_ASSERT(!rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName,"Cache-Control"_sb)]);
            UNIT_ASSERT(rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "Cookie"_sb)]);
            UNIT_ASSERT(rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "Connection"_sb)]);
            UNIT_ASSERT(!rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "Content-Length"_sb)]);
            UNIT_ASSERT(!rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "Content-Type"_sb)]);
            UNIT_ASSERT(rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "Referer"_sb)]);
            UNIT_ASSERT(rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "User-Agent"_sb)]);
            UNIT_ASSERT(!rf.HttpHeaderPresent[FindArrayIndex(CacherHttpHeaderName, "X-Forwarded-For"_sb)]);

            UNIT_ASSERT(!rf.CookiePresent[FindArrayIndex(CacherCookieName, "L"_sb)]);
            UNIT_ASSERT(!rf.CookiePresent[FindArrayIndex(CacherCookieName, "my"_sb)]);
            UNIT_ASSERT(!rf.CookiePresent[FindArrayIndex(CacherCookieName, "Session_id"_sb)]);
            UNIT_ASSERT(rf.CookiePresent[FindArrayIndex(CacherCookieName, "yabs-frequency"_sb)]);
            UNIT_ASSERT(!rf.CookiePresent[FindArrayIndex(CacherCookieName, "yandex_login"_sb)]);
            UNIT_ASSERT(rf.CookiePresent[FindArrayIndex(CacherCookieName, "yandexuid"_sb)]);
            UNIT_ASSERT(!rf.CookiePresent[FindArrayIndex(CacherCookieName, "ys"_sb)]);
            UNIT_ASSERT(rf.CookiePresent[FindArrayIndex(CacherCookieName, "fuid01"_sb)]);
            UNIT_ASSERT_EQUAL(rf.FraudJa3, 0.3F);
            UNIT_ASSERT_EQUAL(rf.FraudSubnet, 0.4F);
            UNIT_ASSERT(rf.HasValidSpravka);
        }

         Y_UNIT_TEST(TestAutoRu) {
            {
                TString req =
                    "GET /autoru?text=hello&lr=123&numdoc=25&p=3 HTTP/1.1\r\n"
                    "Host: antirobot.yandex.ru\r\n"
                    "X-Antirobot-Service-Y: autoru\r\n"
                    "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; GTB6.5; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; InfoPath.2)\r\n"
                    "X-Forwarded-For-Y: 81.18.115.137\r\n"
                    "X-Yandex-Ja3: 771,49195-49196-52393-49199-49200-52392-49171-49172-156-157-47-53,65281-0-23-35-13-5-16-11-10,29-23-24,0\r\n"
                    "\r\n";

                const TCacherRequestFeatures rf = MakeCacherReqFeatures(req);

                UNIT_ASSERT_EQUAL(rf.AutoruJa3, 0.5F);
                UNIT_ASSERT_EQUAL(rf.AutoruSubnet, 0.6F);
                UNIT_ASSERT_EQUAL(rf.AutoruUA, 0.7F);
            }
            {
                TString req =
                    "GET /autoru?text=hello&lr=123&numdoc=25&p=3 HTTP/1.1\r\n"
                    "Host: antirobot.yandex.ru\r\n"
                    "X-Antirobot-Service-Y: autoru\r\n"
                    "User-Agent: Mozilla/4.1 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; GTB6.5; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; InfoPath.2)\r\n"
                    "X-Forwarded-For-Y: 81.18.116.137\r\n"
                    "X-Yandex-Ja3: 772,49195-49196-52393-49199-49200-52392-49171-49172-156-157-47-53,65281-0-23-35-13-5-16-11-10,29-23-24,0\r\n"
                    "\r\n";

                const TCacherRequestFeatures rf = MakeCacherReqFeatures(req);

                UNIT_ASSERT(std::isnan(rf.AutoruJa3));
                UNIT_ASSERT(std::isnan(rf.AutoruSubnet));
                UNIT_ASSERT(std::isnan(rf.AutoruUA));
            }
         }


        Y_UNIT_TEST(TestNoneDicts) {
            TString req =
                "GET /yandsearch?text=hello&lr=123&numdoc=25&p=3 HTTP/1.1\r\n"
                "Accept: image/gif, image/jpeg, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                "Referer: http://yAndeX.kz/yandsearch?text=%D0%B8%D0%BC%D1%8F+%D0%B2%D1%81%D0%B5%D1%85+%D0%B0%D0%BA%D1%82%D0%B5%D1%80%D0%BE%D0%B2+%D0%B8%D0%B7+%D1%81%D0%B5%D1%80%D0%B8%D0%B0%D0%BB%D0%B0+%D0%BF%D1%80%D0%BE%D0%BA%D0%BB%D1%8F%D1%82%D1%8B%D0%B9+%D1%80%D0%B0%D0%B9&lr=63\r\n"
                "Accept-Language: ru\r\n"
                "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0; GTB6.5; SLCC1; .NET CLR 2.0.50727; .NET CLR 3.0.30729; .NET CLR 3.5.30729; InfoPath.2)\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "Host: yandex.kz\r\n"
                "Connection: Keep-Alive\r\n"
                "X-Yandex-ICookie: i_cookie\r\n"
                "Cookie: yandexuid=4151958451269067436; yabs-frequency=/3/rUCLPOm38NES56MC0o5kzXLbZ0CXjsuKPOm3OdVf5MMC0o4YymjbV0Ey//f0BZ01i3W0600O01X0E50uG10W00/fGBZ01m4W0600O010m600G420e410O01X0820W00; fuid01=4ba78514086b657f.DQ7lG1s0WWb5_1mEyZ1hHnsaBvUYivoP7BXtRGy2_ok6ABwwBd6FFsxJc0SMMIn-b81U67nexuq_TgVdyOJhabip6wPer6v8WbItnB2SyUtBlTAiCIZ9q_4muAYVHOO0; spravka=dD0xMjgwOTE5Njc1O2k9ODEuMTguMTE1LjE0Mjt1PTEyODA5MTk2NzUwMDA4NzA2NTM7aD02NTc5MWJhNjIzMDY4YTAwMWExMDQ4ZTJlNmFlNTIyYg%3D%3D\r\n"
                "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                "X-Forwarded-For-Y: 81.18.116.137\r\n"
                "X-Source-Port-Y: 18671\r\n"
                "X-Start-Time: 1280969722810839\r\n"
                "X-Yandex-Ja3: 771,49195-49186-52393-49199-49200-52392-49171-49172-156-157-47-53,65281-0-23-35-13-5-16-11-10,29-23-24,0\r\n"
                "\r\n";

            const TCacherRequestFeatures rf = MakeCacherReqFeatures(req);
            UNIT_ASSERT(std::isnan(rf.FraudJa3));
            UNIT_ASSERT(std::isnan(rf.FraudSubnet));

        }

        Y_UNIT_TEST(TestHoneypots) {
            TString reqTemplate =
                "GET %s HTTP/1.1\r\n"
                "User-Agent: Test\r\n"
                "Host: %s\r\n"
                "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                "X-Forwarded-For-Y: 81.18.116.137\r\n"
                "X-Start-Time: 1280969722810839\r\n"
                "\r\n";

            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/film/1443803?qwe=1", "kinopoisk.ru"));
                UNIT_ASSERT(rf->KinopoiskFilmsHoneypots > 0);
                UNIT_ASSERT(rf->KinopoiskNamesHoneypots <= 0);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/film/555?qwe=1", "kinopoisk.ru"));
                UNIT_ASSERT(rf->KinopoiskFilmsHoneypots <= 0);
                UNIT_ASSERT(rf->KinopoiskNamesHoneypots <= 0);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/name/6261374?qwe=1", "kinopoisk.ru"));
                UNIT_ASSERT(rf->KinopoiskFilmsHoneypots <= 0);
                UNIT_ASSERT(rf->KinopoiskNamesHoneypots > 0);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/name/555?qwe=1", "kinopoisk.ru"));
                UNIT_ASSERT(rf->KinopoiskFilmsHoneypots <= 0);
                UNIT_ASSERT(rf->KinopoiskNamesHoneypots <= 0);
            }
            {
                // check with slash
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/film/1443803/", "kinopoisk.ru"));
                UNIT_ASSERT(rf->KinopoiskFilmsHoneypots > 0);
            }

            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/uslugi/api/get_worker_phone?qwe=1", "uslugi.yandex.ru"));
                UNIT_ASSERT(rf->UslugiGetWorkerPhoneHoneypots);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/api/get_worker_phone/?qwe=1", "uslugi.yandex.ru"));
                UNIT_ASSERT(rf->UslugiGetWorkerPhoneHoneypots);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/uslugi/api/aaa?qwe=1", "uslugi.yandex.ru"));
                UNIT_ASSERT(!rf->UslugiGetWorkerPhoneHoneypots);
            }

            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/some/api/product/77", "market.yandex.ru"));
                UNIT_ASSERT(!rf->MarketHuman1Honeypots);
                UNIT_ASSERT(!rf->MarketHuman2Honeypots);
                UNIT_ASSERT(!rf->MarketHuman3Honeypots);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/api/product/77", "market.yandex.ru"));
                UNIT_ASSERT(rf->MarketHuman1Honeypots);
                UNIT_ASSERT(!rf->MarketHuman2Honeypots);
                UNIT_ASSERT(!rf->MarketHuman3Honeypots);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/product/77/videos/555", "market.yandex.ru"));
                UNIT_ASSERT(!rf->MarketHuman1Honeypots);
                UNIT_ASSERT(rf->MarketHuman2Honeypots);
                UNIT_ASSERT(!rf->MarketHuman3Honeypots);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/my/77/", "market.yandex.ru"));
                UNIT_ASSERT(!rf->MarketHuman1Honeypots);
                UNIT_ASSERT(!rf->MarketHuman2Honeypots);
                UNIT_ASSERT(rf->MarketHuman3Honeypots);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/api/get_worker_phone", "uslugi.yandex.ru"));
                UNIT_ASSERT(rf->UslugiGetWorkerPhoneHoneypots);
                UNIT_ASSERT(!rf->UslugiOtherApiHoneypots);
            }
            for (const auto api : {"map", "get_views_count_for_orders", "get_products_to_buy", "get_worker_active_reaction_packs"}) {
                TString url = TString("/api/") + api;
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), url.c_str(), "uslugi.yandex.ru"));
                UNIT_ASSERT(!rf->UslugiGetWorkerPhoneHoneypots);
                UNIT_ASSERT(rf->UslugiOtherApiHoneypots);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/foo/bar/sale/1101235346-ff93fae1", "auto.ru"));
                UNIT_ASSERT(rf->AutoruOfferHoneypot);
            }
            {
                const auto rf = MakeReqFeatures(Sprintf(reqTemplate.data(), "/foo/bar/sale/1101235346-ff93fae2", "auto.ru"));
                UNIT_ASSERT(!rf->AutoruOfferHoneypot);
            }
        }
        Y_UNIT_TEST(TestAcceptFeatures) {
            TString req =
                "GET /film/1443803?qwe=1 HTTP/1.1\r\n"
                "User-Agent: Test\r\n"
                "Host: kinopoisk.ru\r\n"
                "Accept: image/gif, image/gif, image/pjpeg, application/x-ms-application, application/vnd.ms-xpsdocument, application/xaml+xml, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, application/x-shockwave-flash, */*\r\n"
                "Accept-Language: ru\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "X-Req-Id: 1281969722810839-4606296925164062368\r\n"
                "X-Forwarded-For-Y: 81.18.116.137\r\n"
                "X-Start-Time: 1280969722810839\r\n"
                "\r\n";
            const auto rf = MakeReqFeatures(req);

            UNIT_ASSERT(rf->AcceptUniqueKeysNumber == 11);
            UNIT_ASSERT(rf->AcceptEncodingUniqueKeysNumber == 2);
            UNIT_ASSERT(std::isnan(rf->AcceptCharsetUniqueKeysNumber));
            UNIT_ASSERT(rf->AcceptLanguageUniqueKeysNumber == 1);

            UNIT_ASSERT(rf->AcceptAnySpace);
            UNIT_ASSERT(rf->AcceptEncodingAnySpace);
            UNIT_ASSERT(std::isnan(rf->AcceptCharsetAnySpace));
            UNIT_ASSERT(!rf->AcceptLanguageAnySpace);

            UNIT_ASSERT(rf->AcceptLanguageHasRussian);
        }
    };
}  // namespace NAntiRobot
