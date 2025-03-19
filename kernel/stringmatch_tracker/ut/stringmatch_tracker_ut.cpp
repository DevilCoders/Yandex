#include <kernel/stringmatch_tracker/tracker.h>
#include <kernel/stringmatch_tracker/translit_preparer.h>
#include <kernel/stringmatch_tracker/lcs_wrapper.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NStringMatchTracker;
using TWD = TTextWithDescription;

namespace {
    bool EqFloat(float f1, float f2) {
        return fabs(f1 - f2) < 1e-4;
    }
}

/*
 * Tests for TStringMatchTracker
 */
Y_UNIT_TEST_SUITE(TStringMatchTrackerTest) {

    void Check2VectorsEq(const TVector<float>& v1, const TVector<TFeatureDescription>& v2) {
        for (size_t i = 0; i < v1.size(); ++i) {
            UNIT_ASSERT_EQUAL(EqFloat(v1[i], v2[i].Value), true);
        }
    }

    // Tests if everything is running ok
    Y_UNIT_TEST(TestOfFunctionality) {
        TStringMatchTracker tracker(TStringMatchTracker::CreateDefaultCalcers(), MakeSimpleShared<TTranslitPreparer>());
        tracker.NewQuery(TWD("привет").Set(LANG_RUS));
        tracker.NewDoc(TWD("приветствую, мир!").Set(LANG_RUS));
        tracker.NewQuery(TWD("проверка, проверка").Set(LANG_RUS));
        tracker.NewDoc(TWD("проверяю функциональность системы").Set(LANG_RUS));
        tracker.NewDoc(TWD("38мтк роверка, проверил").Set(LANG_RUS));
        auto v = tracker.CalcFeatures();
        UNIT_ASSERT_EQUAL(v.ysize(), 15);
    }

    // Tests correctness on usual examples
    Y_UNIT_TEST(CorrectnessTest) {
        TStringMatchTracker tracker(TStringMatchTracker::CreateDefaultCalcers());
        tracker.SetPreparer(MakeSimpleShared<TTranslitPreparer>());
        tracker.NewQuery(TWD("привет").Set(LANG_RUS));
        tracker.NewDoc(TWD("приветствую, мир!").Set(LANG_RUS));
        auto calc1 = tracker.CalcFeatures();
        UNIT_ASSERT_EQUAL(calc1.ysize(), 15);

        TVector<float> results1 = {1.f, 6.f / 16.f, 6.f / 16.f, 6.f / 22.f};
        Check2VectorsEq(results1, calc1);

        tracker.NewQuery(TWD("проверка, проверка").Set(LANG_RUS)); // 17
        tracker.NewDoc(TWD("проверяю функциональность системы").Set(LANG_RUS)); // 35 (after translit)

        auto calc2 = tracker.CalcFeatures();
        UNIT_ASSERT_EQUAL(calc2.ysize(), 15);

        TVector<float> results2 = {6.f / 17.f, 6.f / 35.f, 6.f / 35.f, 6.f / 52.f};
        Check2VectorsEq(results2, calc2);

        tracker.NewDoc(TWD("38мтк роверка проверил").Set(LANG_RUS)); // 23
        auto calc3 = tracker.CalcFeatures();
        UNIT_ASSERT_EQUAL(calc3.ysize(), 15);

        TVector<float> results3 = {13.f / 17.f, 13.f / 23.f, 13.f / 23.f, 13.f / 40.f};
        Check2VectorsEq(results3, calc3);
    }

    // Reset Test
    Y_UNIT_TEST(ResetTest) {
        TStringMatchTracker tracker(TStringMatchTracker::CreateDefaultCalcers());
        tracker.SetPreparer(MakeSimpleShared<TTranslitPreparer>());
        tracker.NewQuery(TWD("проверка, проверка").Set(LANG_RUS)); // 17

        tracker.NewDoc(TWD("38мтк роверка проверил").Set(LANG_RUS)); // 23
        tracker.NewDoc(TWD("проверяю функциональность системы").Set(LANG_RUS)); // 33
        TVector<float> results1 = {13.f / 17.f, 13.f / 23.f, 13.f / 23.f, 13.f / 40.f};
        auto calc1 = tracker.CalcFeatures();
        Check2VectorsEq(results1, calc1);

        tracker.ResetDoc();
        tracker.NewDoc(TWD("проверяю функциональность системы").Set(LANG_RUS)); // 33
        auto calc2 = tracker.CalcFeatures();
        TVector<float> results2 = {6.f / 17.f, 6.f / 35.f, 6.f / 35.f, 6.f / 52.f};
        Check2VectorsEq(results2, calc2);
    }

    // Check AddCalcer functionality
    Y_UNIT_TEST(AddCalcerTest) {
        TStringMatchTracker tracker;
        tracker.SetPreparer(MakeSimpleShared<TTranslitPreparer>());
        tracker.NewQuery(TWD("привет").Set(LANG_RUS));
        tracker.NewDoc(TWD("и тебе привет").Set(LANG_RUS));
        UNIT_ASSERT_EQUAL(tracker.CalcFeatures().size(), 0);

        tracker.AddCalcer(MakeSimpleShared<TLCSCalcer>());
        tracker.NewQuery(TWD("привет").Set(LANG_RUS));
        tracker.NewDoc(TWD("и тебе привет").Set(LANG_RUS));
        UNIT_ASSERT_EQUAL(tracker.CalcFeatures().size(), 5);
    }

    // Check on english strings
    Y_UNIT_TEST(EnglishLangTest) {
        TVector<TSimpleSharedPtr<ICalcer>> calcers;
        calcers.push_back(MakeSimpleShared<TLCSCalcer>());
        TStringMatchTracker tracker(calcers);
        tracker.NewQuery(TString("hello"));
        tracker.NewDoc(TWD("world, hello, world"));
        Check2VectorsEq({1.f}, tracker.CalcFeatures());
    }
}
