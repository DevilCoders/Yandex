#include "plan.h"

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/http/io/stream.h>

#include <util/stream/null.h>

Y_UNIT_TEST_SUITE(DolbiloPlanTests) {

    Y_UNIT_TEST(FromRequestCustomPort) {
        TDevastateRequest request;
        request.Url = "http://someurl.com:12345/long/path?query=string";
        TDevastateItem item(TDuration::Days(365), request, 0);

        UNIT_ASSERT_EQUAL(item.Port(), 12345);

        THttpOutput out(&Cnull);
        out << item.Data();
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Host"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Host")->Value(), "someurl.com:12345");
    }

    Y_UNIT_TEST(FromRequestCustomScheme) {
        TDevastateRequest request;
        request.Url = "http2://someurl.com/long/path?query=string";
        TDevastateItem item(TDuration::Days(365), request, 0);

        UNIT_ASSERT_EQUAL(item.Host(), "someurl.com");

        THttpOutput out(&Cnull);
        out << item.Data();
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Host"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Host")->Value(), "someurl.com");
    }

    Y_UNIT_TEST(FromRequestGet) {
        TDevastateRequest request;
        request.Url = "http://someurl.com/long/path?query=string";
        TDevastateItem item(TDuration::Days(365), request, 0);

        UNIT_ASSERT_EQUAL(item.Host(), "someurl.com");
        UNIT_ASSERT_EQUAL(item.Port(), 80);

        THttpOutput out(&Cnull);
        out << item.Data();
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Host"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Host")->Value(), item.Host());
        UNIT_ASSERT_EQUAL(out.FirstLine().StartsWith("GET /long/path?query=string "), true);
    }

    Y_UNIT_TEST(FromRequestPost) {
        TDevastateRequest request;
        request.Url = "http://someurl.com:443/long/path";
        request.Body = { 0, 0, 0, 0 };
        TDevastateItem item(TDuration::Days(365), request, 0);

        UNIT_ASSERT_EQUAL(item.Host(), "someurl.com");
        UNIT_ASSERT_EQUAL(item.Port(), 443);

        THttpOutput out(&Cnull);
        out << item.Data();
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Host"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Host")->Value(), "someurl.com:443");
        UNIT_ASSERT_EQUAL(out.FirstLine().StartsWith("POST /long/path "), true);
        UNIT_ASSERT_EQUAL(out.SentSize(), request.Body.size());
    }

    Y_UNIT_TEST(FromRequestEmptyPath) {
        TDevastateRequest request;
        request.Url = "http://someurl.com";
        TDevastateItem item(TDuration::Days(365), request, 0);

        UNIT_ASSERT_EQUAL(item.Host(), "someurl.com");

        THttpOutput out(&Cnull);
        out << item.Data();
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Host"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Host")->Value(), "someurl.com");
        UNIT_ASSERT_EQUAL(out.FirstLine().StartsWith("GET / "), true);
    }

    Y_UNIT_TEST(FromRequestCustomHeaders) {
        TDevastateRequest request;
        request.Url = "http://someurl.com";
        request.Headers.AddHeader(THttpInputHeader("Name", "Value"));
        request.Headers.AddHeader(THttpInputHeader("Host", "someotherurl.com"));

        TDevastateItem item(TDuration::Days(365), request, 0);

        UNIT_ASSERT_EQUAL(item.Host(), "someurl.com");

        THttpOutput out(&Cnull);
        out << item.Data();
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Host"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Host")->Value(), "someotherurl.com");
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Name"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Name")->Value(), "Value");
    }

    Y_UNIT_TEST(FromRequestInvalidCharacterEncoded) {
        TDevastateRequest request;
        request.Url = "http://someurl.com/path?char=^";
        TDevastateItem item(TDuration::Days(365), request, 0);

        UNIT_ASSERT_EQUAL(item.Host(), "someurl.com");

        THttpOutput out(&Cnull);
        out << item.Data();
        UNIT_ASSERT_EQUAL(out.FirstLine().StartsWith("GET /path?char=%5E"), true);
    }

    Y_UNIT_TEST(FromUrlAndHeaders) {
        TDevastateItem item("http://someurl.com:12345/long/path?query=string",TDuration::Days(365), "Name: Value\nOtherName: OtherValue", 0);

        UNIT_ASSERT_EQUAL(item.Host(), "someurl.com");
        UNIT_ASSERT_EQUAL(item.Port(), 12345);

        THttpOutput out(&Cnull);
        out << item.Data();
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Host"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Host")->Value(), "someurl.com:12345");
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("Name"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("Name")->Value(), "Value");
        UNIT_ASSERT_EQUAL(out.SentHeaders().HasHeader("OtherName"), true);
        UNIT_ASSERT_EQUAL(out.SentHeaders().FindHeader("OtherName")->Value(), "OtherValue");
    }

    Y_UNIT_TEST(FromRawDataInvalid) {
        TString invalidRequest = "Hello";

        UNIT_ASSERT_EXCEPTION(TDevastateItem(TDuration::Days(365), "someurl.com", 80, invalidRequest, 0), yexception);
    }
}
