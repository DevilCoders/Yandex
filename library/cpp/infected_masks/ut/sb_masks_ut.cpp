#include <library/cpp/infected_masks/infected_masks.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <util/generic/algorithm.h>

class TSafeBrowsingMasksTest: public TTestBase {
    UNIT_TEST_SUITE(TSafeBrowsingMasksTest);
    UNIT_TEST(Test);
    UNIT_TEST(TestEscapes);
    UNIT_TEST(TestSub);
    UNIT_TEST(TestLegacy);
    UNIT_TEST(TestReceivingMatches);
    UNIT_TEST(TestInitializer);
    UNIT_TEST(TestSaveLoad);
    UNIT_TEST_SUITE_END();

public:
    void Test();
    void TestEscapes();
    void TestSub();
    void TestLegacy();
    void TestReceivingMatches();
    void TestInitializer();
    void TestSaveLoad();
};

UNIT_TEST_SUITE_REGISTRATION(TSafeBrowsingMasksTest);

void TSafeBrowsingMasksTest::Test() {
    TSafeBrowsingMasks masks;
    masks.Init(GetArcadiaTestsData() + "/infected_masks/sb_masks");

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example0.com/1/foo"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example0.com/1/"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example0.com/1"), false); // Bug fixed

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/1/foo"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/1/"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/1"), false);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/1?key=%7B%7D"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/1?key={}"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/2?key=%7b%7d"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/2?key={}"), true);

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://foo.sub.example0.com/1/"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://foo.sub.example1.com/1/"), true);

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://foo-sub.example0.com/1/"), false); // Bug fixed
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://foo-sub.example1.com/1/"), false);

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example0.com/nosub"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/nosub"), true);

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example0.com/nosub?haha=da"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/nosub?haha=da"), true);

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example0.com/nosub/"), false); // Bug, but consistent with TInfectedMasks. Fixed.
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/nosub/"), false); // Follows spec, inconsistent

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example0.com/nosubfoo"), false); // Bug fixed
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/nosubfoo"), false);

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example0.com/nosub/foo"), false); // Bug fixed
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example1.com/nosub/foo"), false);
}

void TSafeBrowsingMasksTest::TestEscapes() {
    TSafeBrowsingMasks masks;
    masks.Init(GetArcadiaTestsData() + "/infected_masks/sb_masks");

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example5.com/1?key=%7b%7d"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example5.com/1?key=%7B%7D"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example5.com/1?key={}"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example5.com/2?key=%7b%7d"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example5.com/2?key=%7B%7D"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://sub.example5.com/2?key={}"), true);
}

void TSafeBrowsingMasksTest::TestSub() {
    TSafeBrowsingMasks masks;

    {
        THolder<TSafeBrowsingMasks::IInitializer> initializer(masks.GetInitializer());
        initializer->AddMask("example.com/1/", "");
        initializer->Finalize();
    }

    TSafeBrowsingMasks::TMatchesType matches;
    bool result;

    result = masks.IsInfectedUrl("http://sub.example.com/1/2/3");
    UNIT_ASSERT(result);
}

void TSafeBrowsingMasksTest::TestLegacy() {
    TSafeBrowsingMasks masks;
    masks.Init(GetArcadiaTestsData() + "/infected_masks/infected_masks");

    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://far.ru"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://www.far.ru"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://malinkai.esosed.ru"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://malinkai.esosed.ru/"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://malinkai.esosed.ru/blah"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://bbc.co.uk/russian/"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://news.bbc.co.uk/russian/"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://www.bbc.co.uk/russian/zzz"), true);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://bbc.co.uk"), false);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://bbc.co.uk/aaa"), false);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://news.bbc.co.uk"), false);
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://flash-mx.ru/lessons/"), true);

    // This test differs from TInfectedMasks
    UNIT_ASSERT_EQUAL(masks.IsInfectedUrl("http://flash-mx.ru/lessons"), false);
}

bool CheckMatches(TSafeBrowsingMasks::TMatchesType& matches, size_t n, ...) {
    if (matches.size() != n) {
        Cerr << "FAIL: Matches size=" << matches.size() << ", expected=" << n << Endl;
        return false;
    }

    va_list ap;
    va_start(ap, n);
    for (size_t i = 0; i < n; ++i) {
        const char* mask = va_arg(ap, const char*);

        if (FindIf(matches.begin(), matches.end(), [mask](const TSafeBrowsingMasks::TMatchesType::value_type& value) { return value.first == mask; }) == matches.end()) {
            Cerr << "FAIL: Mask '" << mask << "' was not found in matches." << Endl;
            return false;
        }
    }

    return true;
}

bool CheckValues(TSafeBrowsingMasks const& masks, TSafeBrowsingMasks::TMatchesType& matches, size_t n, ...) {
    TVector<TString> values;
    for (auto const& match : matches) {
        masks.ReadValues(match.second, values);
    }

    if (values.size() != n) {
        Cerr << "FAIL: Values size=" << values.size() << ", expected=" << n << Endl;
        return false;
    }

    va_list ap;
    va_start(ap, n);
    for (size_t i = 0; i < n; ++i) {
        const char* expected = va_arg(ap, const char*);

        if (FindIf(values.begin(), values.end(), [expected](TString const& value) { return value == expected; }) == values.end()) {
            Cerr << "FAIL: Value '" << expected << "' was not found in matches." << Endl;
            return false;
        }
    }

    return true;
}

void TSafeBrowsingMasksTest::TestReceivingMatches() {
    TSafeBrowsingMasks masks;
    masks.Init(GetArcadiaTestsData() + "/infected_masks/sb_masks");

    TSafeBrowsingMasks::TMatchesType matches;
    bool result;

    result = masks.IsInfectedUrl("http://sub.sub.example2.com/there/it/is?nocache=da", &matches);
    UNIT_ASSERT(result == true &&
                CheckMatches(matches, 3, "sub.example2.com/", "sub.sub.example2.com/there/", "sub.sub.example2.com/there/it/is"));
    UNIT_ASSERT(result == true &&
                CheckValues(masks, matches, 3, "data-14", "data-17", "data-18"));

    result = masks.IsInfectedUrl("http://sup.sub.example2.com/there/it/is?nocache=da", &matches);
    UNIT_ASSERT(result == true &&
                CheckMatches(matches, 2, "sub.example2.com/", "sup.sub.example2.com/there/"));
    UNIT_ASSERT(result == true &&
                CheckValues(masks, matches, 3, "data-14", "data-15", "data-16"));

    result = masks.IsInfectedUrl("http://sub.sub.example3.com/there/it/is?nocache=da", &matches);
    UNIT_ASSERT(result == true &&
                CheckMatches(matches, 6, "example3.com/", "sub.example3.com/", "sub.sub.example3.com/there/it/is",
                             "sub.sub.example3.com/there/", "sub.sub.example3.com/there/it/", "example3.com/there/"));
    UNIT_ASSERT(result == true &&
                CheckValues(masks, matches, 6, "data-19", "data-20", "data-22", "data-25", "data-26", "data-27"));

    result = masks.IsInfectedUrl("http://sub.sub.example4.com/there/it/is?nocache=da", &matches);
    UNIT_ASSERT(result == true && CheckMatches(matches, 1, "example4.com/"));
    UNIT_ASSERT(result == true &&
                CheckValues(masks, matches, 1, "data-29"));

    result = masks.IsInfectedUrl("http://sub.example1.com/1?key=%7B%7D", &matches);
    UNIT_ASSERT(result == true && CheckMatches(matches, 1, "sub.example1.com/1?key={}"));
    UNIT_ASSERT(result == true &&
                CheckValues(masks, matches, 1, "data-32"));
}

void TSafeBrowsingMasksTest::TestInitializer() {
    TSafeBrowsingMasks masks;

    {
        THolder<TSafeBrowsingMasks::IInitializer> initializer(masks.GetInitializer());
        initializer->AddMask("sub2.sub.example.com/1/", "");
        initializer->AddMask("sub.example.com/1/2/", "");
        initializer->Finalize();
    }

    TSafeBrowsingMasks::TMatchesType matches;
    bool result;

    result = masks.IsInfectedUrl("http://sub2.sub.example.com/1/2/3", &matches);
    UNIT_ASSERT(result == true &&
                CheckMatches(matches, 2, "sub2.sub.example.com/1/", "sub.example.com/1/2/"));
}

void TSafeBrowsingMasksTest::TestSaveLoad() {
    TString savedMasks;
    {
        TSafeBrowsingMasks initialMasks;
        initialMasks.Init(GetArcadiaTestsData() + "/infected_masks/sb_masks");
        TStringOutput saveStream(savedMasks);
        initialMasks.Save(&saveStream);
    }
    TSafeBrowsingMasks masks;
    {
        TStringInput loadStream(savedMasks);
        masks.Load(&loadStream);
    }

    TSafeBrowsingMasks::TMatchesType matches;
    bool result;

    result = masks.IsInfectedUrl("http://sub.sub.example2.com/there/it/is?nocache=da", &matches);
    UNIT_ASSERT(result == true &&
                CheckMatches(matches, 3, "sub.example2.com/", "sub.sub.example2.com/there/", "sub.sub.example2.com/there/it/is"));
    UNIT_ASSERT(result == true &&
                CheckValues(masks, matches, 3, "data-14", "data-17", "data-18"));

    result = masks.IsInfectedUrl("http://sup.sub.example2.com/there/it/is?nocache=da", &matches);
    UNIT_ASSERT(result == true &&
                CheckMatches(matches, 2, "sub.example2.com/", "sup.sub.example2.com/there/"));
    UNIT_ASSERT(result == true &&
                CheckValues(masks, matches, 3, "data-14", "data-15", "data-16"));
}
