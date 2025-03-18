#include <library/cpp/infected_masks/infected_masks.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

static TVector<TString> GenerateMasks(const TString& url) {
    TVector<TString> res;
    for (TInfectedMasksGenerator generator(url); generator.HasNext(); generator.Next()) {
        res.push_back(generator.Current());
    }
    return res;
}

Y_UNIT_TEST_SUITE(InfectedMasksGeneratorTestSuite) {
    Y_UNIT_TEST(TestGenerator) {
        TString empty;
        TInfectedMasksGenerator generator(empty);
    }

    Y_UNIT_TEST(TInfectedMasksTest) {
        TInfectedMasks masks;
        masks.Init(GetArcadiaTestsData() + "/infected_masks/infected_masks");
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://far.ru"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://www.far.ru"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://malinkai.esosed.ru"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://malinkai.esosed.ru/"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://malinkai.esosed.ru/blah"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://malinkai...esosed..ru/blah"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://bbc.co.uk/russian/"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://news.bbc.co.uk/russian/"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://www.bbc.co.uk/russian/zzz"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://bbc.co.uk"), false);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://bbc.co.uk/aaa"), false);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://news.bbc.co.uk"), false);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://news.bbc...co..uk"), false);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://flash-mx.ru/lessons/"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://flash-mx.ru/lessons"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://flash-mx...ru/lessons"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://..flash-mx.ru/lessons"), true);
        UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://flash-mx.ru../lessons"), true);
    }

    Y_UNIT_TEST(TestSaaSIssue) {
        using namespace NInfectedMasks;
        {
            TVector<TLiteMask> masks;
            GenerateMasksFast(masks, "www.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", C_YANDEX_GOOGLE_ORDER);
            UNIT_ASSERT_VALUES_EQUAL(masks, (TVector<TLiteMask>{
                                                {"www.funnycat.tv", "/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8"},
                                                {"www.funnycat.tv", "/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", "/"},
                                                {"www.funnycat.tv", "/"},
                                                {"www.funnycat.tv", "/video/"},
                                                {"www.funnycat.tv", "/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/"},
                                                {"funnycat.tv", "/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8"},
                                                {"funnycat.tv", "/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", "/"},
                                                {"funnycat.tv", "/"},
                                                {"funnycat.tv", "/video/"},
                                                {"funnycat.tv", "/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/"}}));
        }
        {
            TVector<TLiteMask> masks;
            GenerateMasksFast(masks, "protectload.ru/Keys/klyuchi-dlya-avast-premier-kod-aktivacii-2016.html", C_YANDEX_GOOGLE_ORDER);
            UNIT_ASSERT_VALUES_EQUAL(masks, (TVector<TLiteMask>{
                                                {"protectload.ru", "/Keys/klyuchi-dlya-avast-premier-kod-aktivacii-2016.html"},
                                                {"protectload.ru", "/Keys/klyuchi-dlya-avast-premier-kod-aktivacii-2016.html", "/"},
                                                {"protectload.ru", "/"},
                                                {"protectload.ru", "/Keys/"}}));
        }
        {
            TVector<TLiteMask> masks;
            GenerateMasksFast(masks, "http://abcimagess.com/veins+popping+in+legs", C_YANDEX_GOOGLE_ORDER);
            UNIT_ASSERT_VALUES_EQUAL(masks, (TVector<TLiteMask>{
                                                {"abcimagess.com", "/veins+popping+in+legs"},
                                                {"abcimagess.com", "/veins+popping+in+legs", "/"},
                                                {"abcimagess.com", "/"},
                                            }));
        }
        {
            TVector<TLiteMask> masks;
            GenerateMasksFast(masks, "https://abcimagess.com/veins+popping+in+legs", C_YANDEX_GOOGLE_ORDER);
            UNIT_ASSERT_VALUES_EQUAL(masks, (TVector<TLiteMask>{
                                                {"abcimagess.com", "/veins+popping+in+legs"},
                                                {"abcimagess.com", "/veins+popping+in+legs", "/"},
                                                {"abcimagess.com", "/"},
                                            }));
        }
    }

    Y_UNIT_TEST(TInfectedMasksGeneratorTest) {
        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("http://sub.sub.example2.com/there/it/is?nocache=da&param=param"),
                                 TVector<TString>({"example2.com/",
                                                   "sub.example2.com/",
                                                   "sub.sub.example2.com/",
                                                   "example2.com/there/",
                                                   "sub.example2.com/there/",
                                                   "sub.sub.example2.com/there/",
                                                   "example2.com/there/it/",
                                                   "sub.example2.com/there/it/",
                                                   "sub.sub.example2.com/there/it/",
                                                   "example2.com/there/it/is",
                                                   "sub.example2.com/there/it/is",
                                                   "sub.sub.example2.com/there/it/is",
                                                   "example2.com/there/it/is?nocache=da&param=param",
                                                   "sub.example2.com/there/it/is?nocache=da&param=param",
                                                   "sub.sub.example2.com/there/it/is?nocache=da&param=param"}));

        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("http://sub.sub.example2.com/there/it/is"),
                                 TVector<TString>({
                                     "example2.com/",
                                     "sub.example2.com/",
                                     "sub.sub.example2.com/",
                                     "example2.com/there/",
                                     "sub.example2.com/there/",
                                     "sub.sub.example2.com/there/",
                                     "example2.com/there/it/",
                                     "sub.example2.com/there/it/",
                                     "sub.sub.example2.com/there/it/",
                                     "example2.com/there/it/is",
                                     "sub.example2.com/there/it/is",
                                     "sub.sub.example2.com/there/it/is",
                                     "example2.com/there/it/is/",
                                     "sub.example2.com/there/it/is/",
                                     "sub.sub.example2.com/there/it/is/",
                                 }));

        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("http://sub.sub.example2.com/there/it/is"),
                                 TVector<TString>({
                                     "example2.com/",
                                     "sub.example2.com/",
                                     "sub.sub.example2.com/",
                                     "example2.com/there/",
                                     "sub.example2.com/there/",
                                     "sub.sub.example2.com/there/",
                                     "example2.com/there/it/",
                                     "sub.example2.com/there/it/",
                                     "sub.sub.example2.com/there/it/",
                                     "example2.com/there/it/is",
                                     "sub.example2.com/there/it/is",
                                     "sub.sub.example2.com/there/it/is",
                                     "example2.com/there/it/is/",
                                     "sub.example2.com/there/it/is/",
                                     "sub.sub.example2.com/there/it/is/",
                                 }));

        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("http://a.b.c.d.e.f.g"),
                                 TVector<TString>({"f.g/", "e.f.g/", "d.e.f.g/", "c.d.e.f.g/", "a.b.c.d.e.f.g/"}));

        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("http://a.b.c.d.e.f.g/"),
                                 TVector<TString>({"f.g/", "e.f.g/", "d.e.f.g/", "c.d.e.f.g/", "a.b.c.d.e.f.g/"}));

        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("h/a/b/c/d/e/f/g/"),
                                 TVector<TString>({"h/", "h/a/", "h/a/b/", "h/a/b/c/", "h/a/b/c/d/", "h/a/b/c/d/e/f/g/"}));

        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("h/a/b/c/d/e/f/g"),
                                 TVector<TString>({"h/", "h/a/", "h/a/b/", "h/a/b/c/", "h/a/b/c/d/", "h/a/b/c/d/e/f/g", "h/a/b/c/d/e/f/g/"}));

        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("h/a/b/c/d/e/f/g?x=y"),
                                 TVector<TString>({"h/", "h/a/", "h/a/b/", "h/a/b/c/", "h/a/b/c/d/e/f/g", "h/a/b/c/d/e/f/g?x=y"}));

        UNIT_ASSERT_VALUES_EQUAL(GenerateMasks("h/a/b/c/d/e/f/g/?x=y"),
                                 TVector<TString>({"h/", "h/a/", "h/a/b/", "h/a/b/c/", "h/a/b/c/d/e/f/g/", "h/a/b/c/d/e/f/g/?x=y"}));
    }

    Y_UNIT_TEST(TestGenerateMasksFast) {
        using namespace NInfectedMasks;
        TVector<TLiteMask> masks;

        {
            TString url = "http://a.b.c/1/2.html?param=1";
            GenerateMasksFast(masks, url, C_GOOGLE);

            UNIT_ASSERT_VALUES_EQUAL(masks, (TVector<TLiteMask>{
                                                {"a.b.c", "/1/2.html?param=1"},
                                                {"a.b.c", "/1/2.html"},
                                                {"a.b.c", "/"},
                                                {"a.b.c", "/1/"},
                                                {"b.c", "/1/2.html?param=1"},
                                                {"b.c", "/1/2.html"},
                                                {"b.c", "/"},
                                                {"b.c", "/1/"}}));
        }
        {
            TString url = "http://a.b.c.d.e.f.g/1.html";
            GenerateMasksFast(masks, url, C_GOOGLE);

            UNIT_ASSERT_VALUES_EQUAL(masks, (TVector<TLiteMask>{
                                                {"a.b.c.d.e.f.g", "/1.html"},
                                                {"a.b.c.d.e.f.g", "/"},
                                                {"c.d.e.f.g", "/1.html"},
                                                {"c.d.e.f.g", "/"},
                                                {"d.e.f.g", "/1.html"},
                                                {"d.e.f.g", "/"},
                                                {"e.f.g", "/1.html"},
                                                {"e.f.g", "/"},
                                                {"f.g", "/1.html"},
                                                {"f.g", "/"},
                                            }));
        }
        {
            TString url = "http://a.b.c.d.e.f.g/1.html";
            GenerateMasksFast(masks, url, C_YANDEX_GOOGLE_ORDER);

            UNIT_ASSERT_VALUES_EQUAL(masks, (TVector<TLiteMask>{
                                                {"a.b.c.d.e.f.g", "/1.html"},
                                                {"a.b.c.d.e.f.g", "/1.html", "/"},
                                                {"a.b.c.d.e.f.g", "/"},
                                                {"c.d.e.f.g", "/1.html"},
                                                {"c.d.e.f.g", "/1.html", "/"},
                                                {"c.d.e.f.g", "/"},
                                                {"d.e.f.g", "/1.html"},
                                                {"d.e.f.g", "/1.html", "/"},
                                                {"d.e.f.g", "/"},
                                                {"e.f.g", "/1.html"},
                                                {"e.f.g", "/1.html", "/"},
                                                {"e.f.g", "/"},
                                                {"f.g", "/1.html"},
                                                {"f.g", "/1.html", "/"},
                                                {"f.g", "/"},
                                            }));
        }
        {
            TString url = "http://a.b.c.d.e.f.g/1.html";
            GenerateMasksFast(masks, url, C_YANDEX);

            UNIT_ASSERT_VALUES_EQUAL(masks, (TVector<TLiteMask>{
                                                {"f.g", "/"},
                                                {"e.f.g", "/"},
                                                {"d.e.f.g", "/"},
                                                {"c.d.e.f.g", "/"},
                                                {"a.b.c.d.e.f.g", "/"},
                                                {"f.g", "/1.html"},
                                                {"e.f.g", "/1.html"},
                                                {"d.e.f.g", "/1.html"},
                                                {"c.d.e.f.g", "/1.html"},
                                                {"a.b.c.d.e.f.g", "/1.html"},
                                                {"f.g", "/1.html", "/"},
                                                {"e.f.g", "/1.html", "/"},
                                                {"d.e.f.g", "/1.html", "/"},
                                                {"c.d.e.f.g", "/1.html", "/"},
                                                {"a.b.c.d.e.f.g", "/1.html", "/"},
                                            }));
        }
        for (auto c : {C_GOOGLE, C_YANDEX_GOOGLE_ORDER}) {
            {
                TString url = "https://a.b.c.d.e.f.g/";
                GenerateMasksFast(masks, url, c);
                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"a.b.c.d.e.f.g", "/"},
                                                      {"c.d.e.f.g", "/"},
                                                      {"d.e.f.g", "/"},
                                                      {"e.f.g", "/"},
                                                      {"f.g", "/"},
                                                  }),
                                           c);
            }
            {
                TString url = "https://a.b.c.d.e.f.g";
                GenerateMasksFast(masks, url, c);

                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"a.b.c.d.e.f.g", "/"},
                                                      {"c.d.e.f.g", "/"},
                                                      {"d.e.f.g", "/"},
                                                      {"e.f.g", "/"},
                                                      {"f.g", "/"},
                                                  }),
                                           c);
            }
            {
                TString url = "https://a.b.c.d.e.f.g";
                GenerateMasksFast(masks, url, c);

                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"a.b.c.d.e.f.g", "/"},
                                                      {"c.d.e.f.g", "/"},
                                                      {"d.e.f.g", "/"},
                                                      {"e.f.g", "/"},
                                                      {"f.g", "/"},
                                                  }),
                                           c);
            }
            {
                TString url = "h/a/b/c/d/e/f/g/";
                GenerateMasksFast(masks, url, c);

                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"h", "/a/b/c/d/e/f/g/"},
                                                      {"h", "/"},
                                                      {"h", "/a/"},
                                                      {"h", "/a/b/"},
                                                      {"h", "/a/b/c/"},
                                                      {"h", "/a/b/c/d/"},
                                                  }),
                                           c);
            }
            {
                TString url = "a.b.c.d.e.f.g/a/b/c/d/e/f/g/";
                GenerateMasksFast(masks, url, c);

                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"a.b.c.d.e.f.g", "/a/b/c/d/e/f/g/"},
                                                      {"a.b.c.d.e.f.g", "/"},
                                                      {"a.b.c.d.e.f.g", "/a/"},
                                                      {"a.b.c.d.e.f.g", "/a/b/"},
                                                      {"a.b.c.d.e.f.g", "/a/b/c/"},
                                                      {"a.b.c.d.e.f.g", "/a/b/c/d/"},
                                                      {"c.d.e.f.g", "/a/b/c/d/e/f/g/"},
                                                      {"c.d.e.f.g", "/"},
                                                      {"c.d.e.f.g", "/a/"},
                                                      {"c.d.e.f.g", "/a/b/"},
                                                      {"c.d.e.f.g", "/a/b/c/"},
                                                      {"c.d.e.f.g", "/a/b/c/d/"},
                                                      {"d.e.f.g", "/a/b/c/d/e/f/g/"},
                                                      {"d.e.f.g", "/"},
                                                      {"d.e.f.g", "/a/"},
                                                      {"d.e.f.g", "/a/b/"},
                                                      {"d.e.f.g", "/a/b/c/"},
                                                      {"d.e.f.g", "/a/b/c/d/"},
                                                      {"e.f.g", "/a/b/c/d/e/f/g/"},
                                                      {"e.f.g", "/"},
                                                      {"e.f.g", "/a/"},
                                                      {"e.f.g", "/a/b/"},
                                                      {"e.f.g", "/a/b/c/"},
                                                      {"e.f.g", "/a/b/c/d/"},
                                                      {"f.g", "/a/b/c/d/e/f/g/"},
                                                      {"f.g", "/"},
                                                      {"f.g", "/a/"},
                                                      {"f.g", "/a/b/"},
                                                      {"f.g", "/a/b/c/"},
                                                      {"f.g", "/a/b/c/d/"},
                                                  }),
                                           c);
            }
            {
                TString url = "c.d.e.f.g/a/b/c/d/";
                GenerateMasksFast(masks, url, c);

                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"c.d.e.f.g", "/a/b/c/d/"},
                                                      {"c.d.e.f.g", "/"},
                                                      {"c.d.e.f.g", "/a/"},
                                                      {"c.d.e.f.g", "/a/b/"},
                                                      {"c.d.e.f.g", "/a/b/c/"},
                                                      {"d.e.f.g", "/a/b/c/d/"},
                                                      {"d.e.f.g", "/"},
                                                      {"d.e.f.g", "/a/"},
                                                      {"d.e.f.g", "/a/b/"},
                                                      {"d.e.f.g", "/a/b/c/"},
                                                      {"e.f.g", "/a/b/c/d/"},
                                                      {"e.f.g", "/"},
                                                      {"e.f.g", "/a/"},
                                                      {"e.f.g", "/a/b/"},
                                                      {"e.f.g", "/a/b/c/"},
                                                      {"f.g", "/a/b/c/d/"},
                                                      {"f.g", "/"},
                                                      {"f.g", "/a/"},
                                                      {"f.g", "/a/b/"},
                                                      {"f.g", "/a/b/c/"},
                                                  }),
                                           c);
            }
            {
                TString url = "d.e.f.g/a/b/c/";
                GenerateMasksFast(masks, url, c);

                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"d.e.f.g", "/a/b/c/"},
                                                      {"d.e.f.g", "/"},
                                                      {"d.e.f.g", "/a/"},
                                                      {"d.e.f.g", "/a/b/"},
                                                      {"e.f.g", "/a/b/c/"},
                                                      {"e.f.g", "/"},
                                                      {"e.f.g", "/a/"},
                                                      {"e.f.g", "/a/b/"},
                                                      {"f.g", "/a/b/c/"},
                                                      {"f.g", "/"},
                                                      {"f.g", "/a/"},
                                                      {"f.g", "/a/b/"},
                                                  }),
                                           c);
            }
            {
                TString url = "e.f.g/a/b/";
                GenerateMasksFast(masks, url, c);

                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"e.f.g", "/a/b/"},
                                                      {"e.f.g", "/"},
                                                      {"e.f.g", "/a/"},
                                                      {"f.g", "/a/b/"},
                                                      {"f.g", "/"},
                                                      {"f.g", "/a/"},
                                                  }),
                                           c);
            }
            {
                TString url = "f.g/a/";
                GenerateMasksFast(masks, url, c);

                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{
                                                      {"f.g", "/a/"},
                                                      {"f.g", "/"},
                                                  }),
                                           c);
            }

            // Патологические бессмысленные случаи, главное на них - не падать

            {
                TString url = "a.b?";
                GenerateMasksFast(masks, url, c);
                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{{"a.b", "/"}}), c);
            }
            {
                TString url = ".";
                GenerateMasksFast(masks, url, c);
                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{{".", "/"}}), c);
            }
            {
                TString url = "./";
                GenerateMasksFast(masks, url, c);
                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{{".", "/"}}), c);
            }
            {
                TString url = ".?";
                GenerateMasksFast(masks, url, c);
                UNIT_ASSERT_VALUES_EQUAL_C(masks, (TVector<TLiteMask>{{".", "/"}}), c);
            }
        }
    }
}
