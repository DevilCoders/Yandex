#include <kernel/url_text_analyzer/url_analyzer.h>
#include <kernel/url_text_analyzer/lite/url_analyzer.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestLayers) {
    Y_UNIT_TEST(TestTReLULayer) {
        NUta::TSmartUrlAnalyzer analyzer;
        TVector<TString> result = analyzer.AnalyzeUrlUTF8(
            "https://ru.wikipedia.org/wiki/%D0%AF%D0%BD%D0%B4%D0%B5%D0%BA%D1%81");
        TVector<TString> expectedValue = {"ru", "wikipedia", "wiki", "яндекс"};
        UNIT_ASSERT_VALUES_EQUAL(expectedValue, result);
    }
    Y_UNIT_TEST(TestIntegerTokens) {
        NUta::TOpts opts = NUta::TDefaultSmartSplitOpts();
        opts.IgnoreNumbers = false;
        NUta::TSmartUrlAnalyzer analyzer({}, opts);
        TVector<TString> result = analyzer.AnalyzeUrlUTF8(
            "https://www.google.ru/maps/place/Ливерпуль,+Великобритания/@53.419824,-3.0509283");
        TVector<TString> expectedValue = {"google", "maps", "place", "ливерпуль", "великобритания", "53", "419824", "3", "0509283"};
        UNIT_ASSERT_VALUES_EQUAL(expectedValue, result);
    }
    Y_UNIT_TEST(TestWinCode) {
        NUta::TOpts opts;
        opts.ProcessWinCode = true;
        NUta::TSmartUrlAnalyzer analyzer({}, opts);
        TVector<TString> result = analyzer.AnalyzeUrlUTF8(
            "http://www.puzikov.com/order/?%20%C2.%C0.%20%C1%E0%E7%E0%F0%EE%E2&%20%C3.%CC.%20%CA%F0%E6%E8%E6%E0%ED%EE%E2%F1%EA%E8%E9&%20%D1.%C3.%20%D1%F2%F0%F3%EC%E8%EB%E8%ED%20%E8%20%E4%F0.)&tema=%D3%20%E8%F1%F2%EE%EA%EE%E2%20%EE%F2%E5%F7%E5%F1%F2%E2%E5%ED%ED%EE%E9%20%EF%EB%E0%ED%EE%E2%EE%E9%20%EC%FB%F1%EB%E8%20(%CD.%C4.%20%CA%EE%ED%E4%F0%E0%F2%FC%E5%E2");
        TVector<TString> expectedValue = {"www", "puzikov", "com", "order", "в", "а", "базаров", "г", "м", "кржижановский", "с", "г", "струмилин", "и", "др", "tema", "у", "истоков",
            "отечественной", "плановой", "мысли", "н", "д", "кондратьев"};
        UNIT_ASSERT_VALUES_EQUAL(expectedValue, result);
    }
    Y_UNIT_TEST(TestDoubleSeparator) {
        NUta::TSmartUrlAnalyzer analyzer;
        TVector<TString> result = analyzer.AnalyzeUrlUTF8("http://www.foo.com/yxxx//zxxx?u=vxxx&&w=vxxx2");
        TVector<TString> expectedValue = {"foo", "yxxx", "zxxx", "vxxx", "vxxx2"};
        UNIT_ASSERT_VALUES_EQUAL(expectedValue, result);
    }
    Y_UNIT_TEST(TestUntransliteLite) {
        NUta::TFastStrictOpts opts = NUta::TFastStrictOpts();
        opts.TryUntranslit = true;
        NUta::TLiteUrlAnalyzer analyzer(opts);
        UNIT_ASSERT_EXCEPTION(analyzer.AnalyzeUrlUTF8("http://ya.ru"), yexception);
    }
    Y_UNIT_TEST(TestTReLULayerLite) {
        NUta::TLiteUrlAnalyzer analyzer;
        TVector<TString> result = analyzer.AnalyzeUrlUTF8(
            "https://ru.wikipedia.org/wiki/%D0%AF%D0%BD%D0%B4%D0%B5%D0%BA%D1%81");
        TVector<TString> expectedValue = {"ru", "wikipedia", "wiki", "яндекс"};
        UNIT_ASSERT_VALUES_EQUAL(expectedValue, result);
    }
    Y_UNIT_TEST(TestIntegerTokensLite) {
        NUta::TFastStrictOpts opts = NUta::TFastStrictOpts();
        opts.IgnoreNumbers = false;
        NUta::TLiteUrlAnalyzer analyzer(opts);
        TVector<TString> result = analyzer.AnalyzeUrlUTF8(
            "https://www.google.ru/maps/place/Ливерпуль,+Великобритания/@53.419824,-3.0509283");
        TVector<TString> expectedValue = {"google", "maps", "place", "ливерпуль", "великобритания", "53", "419824", "3", "0509283"};
        UNIT_ASSERT_VALUES_EQUAL(expectedValue, result);
    }
}
