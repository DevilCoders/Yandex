#include <library/cpp/testing/unittest/registar.h>

#include "http_request.h"

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(HttpRequest) {

Y_UNIT_TEST(Constructor) {
    THttpRequest request("GET", "yandex.ru", "/");
    UNIT_ASSERT_STRINGS_EQUAL(ToString(request), "GET / HTTP/1.1\r\n"
                                                 "Host: yandex.ru\r\n"
                                                 "\r\n");
}

Y_UNIT_TEST(AddHeader) {
    THttpRequest request("GET", "yandex.com.tr", "/favicon.ico");
    request.AddHeader("Header1", "Value1");
    request.AddHeader("Header2", "Value2");
    UNIT_ASSERT_STRINGS_EQUAL(ToString(request), "GET /favicon.ico HTTP/1.1\r\n"
                                                 "Host: yandex.com.tr\r\n"
                                                 "Header1: Value1\r\n"
                                                 "Header2: Value2\r\n"
                                                 "\r\n");
}

Y_UNIT_TEST(AddCgiParam) {
    THttpRequest request("GET", "yandex.ua", "/yandsearch");
    request.AddCgiParam("text", "vkontakte");
    request.AddCgiParam("param", "a : b");
    UNIT_ASSERT_STRINGS_EQUAL(ToString(request), "GET /yandsearch?param=a+%3A+b&text=vkontakte HTTP/1.1\r\n"
                                                 "Host: yandex.ua\r\n"
                                                 "\r\n");
}

Y_UNIT_TEST(AddCgiParameters) {
    THttpRequest request("GET", "yandex.ua", "/yandsearch");
    request.AddCgiParam("lr", "123");

    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("text"), TStringBuf("vkontakte"));
    cgi.InsertUnescaped(TStringBuf("param"), TStringBuf("a : b"));

    request.AddCgiParameters(cgi);
    UNIT_ASSERT_STRINGS_EQUAL(ToString(request), "GET /yandsearch?lr=123&param=a+%3A+b&text=vkontakte HTTP/1.1\r\n"
                                                 "Host: yandex.ua\r\n"
                                                 "\r\n");
}

Y_UNIT_TEST(ReturnReference) {
    THttpRequest request("GET", "yandex.ru", "/");
    UNIT_ASSERT_EQUAL(&request, &request.AddHeader("Header", "Value"));
    UNIT_ASSERT_EQUAL(&request, &request.AddCgiParam("param", "value"));
    UNIT_ASSERT_EQUAL(&request, &request.AddCgiParameters(TCgiParameters()));
    UNIT_ASSERT_EQUAL(&request, &request.SetContent(""));
    UNIT_ASSERT_EQUAL(&request, &request.SetHost("ya.ru"));
}

Y_UNIT_TEST(SetContent) {
    const TString xml = R"(<xml><tag param="value">content</tag></xml>)";

    THttpRequest request("POST", "yandex.com.tr", "/search.xml");
    request.SetContent(xml);

    UNIT_ASSERT_STRINGS_EQUAL(ToString(request), "POST /search.xml HTTP/1.1\r\n"
                                                 "Host: yandex.com.tr\r\n"
                                                 "Content-Length: " + ToString(xml.length()) + "\r\n"
                                                 "\r\n" + xml);
}

Y_UNIT_TEST(SetHost) {
    THttpRequest request("GET", "yandex.ru", "/");
    request.SetHost("my.favourite.site.com");

    UNIT_ASSERT_STRINGS_EQUAL(ToString(request), "GET / HTTP/1.1\r\n"
                                                 "Host: my.favourite.site.com\r\n"
                                                 "\r\n");
}

Y_UNIT_TEST(BuildHttpGet) {
    UNIT_ASSERT_STRINGS_EQUAL(ToString(HttpGet("ya.ru", "/")), "GET / HTTP/1.1\r\n"
                                                               "Host: ya.ru\r\n"
                                                               "\r\n");
}

Y_UNIT_TEST(BuildHttpPost) {
    UNIT_ASSERT_STRINGS_EQUAL(ToString(HttpPost("ya.ru", "/")), "POST / HTTP/1.1\r\n"
                                                                "Host: ya.ru\r\n"
                                                                "\r\n");
}

Y_UNIT_TEST(ToLogString) {
    THttpRequest request("GET", "yandex.ru", "/");
    UNIT_ASSERT_STRINGS_EQUAL(request.ToLogString(), "GET / HTTP/1.1\\r\\n"
                                                     "Host: yandex.ru\\r\\n"
                                                     "\\r\\n");
}

}

}
