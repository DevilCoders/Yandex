#include "factors.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/ut_utils/scheme_ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/join.h>
#include <util/string/printf.h>

using namespace NWizardsClicks;
using namespace NSc::NUt;

class TWizardClicksFactorsTest : public NUnitTest::TTestBase {
    UNIT_TEST_SUITE(TWizardClicksFactorsTest);
        UNIT_TEST(EmptyTest);
        UNIT_TEST(UnknownWizardTest);
        UNIT_TEST(NonPosTest);
        UNIT_TEST(PosTest);
        UNIT_TEST(AggrTest);
    UNIT_TEST_SUITE_END();

private:
    void AppendFactor(TVector<double>&) {
        return;
    }

    template <typename... TRest>
    void AppendFactor(TVector<double>& res, double f, const TRest&... parameters) {
        res.push_back(f);
        return AppendFactor(res, parameters...);
    }

    TString Print(const TVector<double>& a, const TVector<double>& b) {
        return Sprintf("[%s] != [%s]", JoinSeq(" ", a).data(), JoinSeq(" ", b).data());
    }

    template <typename... TRest>
    void AssertFactorsEqual(const TTokenFactorCalculator& fc, const TRest&... factors) {
        const auto& res = CalcFactors(fc);
        TVector<double> etalon;
        AppendFactor(etalon, factors...);
        static double eps = 1e-6;
        UNIT_ASSERT_EQUAL_C(res.size(), etalon.size(), "different factors size");
        for (size_t i = 0; i < res.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL_C(etalon[i], res[i], eps, Sprintf(", index=%lu, %s", i, Print(etalon, res).data()));
        }
    }

    TVector<double> CalcFactors(const TTokenFactorCalculator& fc) {
        TVector<double> res;
        res.push_back(fc.CTR0());
        res.push_back(fc.CorrectedCTR15());
        res.push_back(fc.FRC15());
        res.push_back(fc.WeightCTR());
        res.push_back(fc.WeightFRC());
        res.push_back(fc.SkipToShows());
        res.push_back(fc.NavigCTR0());
        res.push_back(0.5 * (1.0 + fc.SurplusToShows()));
        res.push_back(fc.ImagesServiceRequestContrast());
        return res;
    }

public:
    void EmptyTest() {
        NSc::TValue v;
        TTokenFactorCalculator fc(&v, "images", "images", 1.f);
        AssertFactorsEqual(fc, 0, 0, 0, 0, 0, 0, 0, 0.5, 0);
    }

    void UnknownWizardTest() {
        NSc::TValue v = AssertFromJson(Json);
        TTokenFactorCalculator fc(&v, "bla", "bla", 1.f);
        AssertFactorsEqual(fc, 0, 0, 0, 0, 0.f, 0, 0, 0.5, 0.19375);
    }

    void NonPosTest() {
        NSc::TValue v = AssertFromJson(Json);
        TTokenFactorCalculator ifc(&v, "images", "images", 1.f);
        AssertFactorsEqual(ifc, 0.547619, 0.917808, 0.905405, 0.517761, 0.914774, 0.02381, 0.085271, 0.7619047619, 0.19375);

        TTokenFactorCalculator vfc(&v, "video", "video", 1.f);
        AssertFactorsEqual(vfc, 0.008064, 0, 0, 0.0010067, 0.020124, 0.016129, 0.007752, 0.4959677, 0.19375);
    }

    void PosTest() {
        NSc::TValue v = AssertFromJson(Json);

        TTokenFactorCalculator fc0(&v, "images", "images", 1.f, 0);
        AssertFactorsEqual(fc0, 0.542373, 0.9117647, 0.898551, 0.512321, 0.908646, 0.025424, 0.085271, 0.7584745763, 0.19375);

        TTokenFactorCalculator fc1(&v, "images", "images", 1.f, 1);
        AssertFactorsEqual(fc1, 0.625, 0.833333, 0.416667, 0.597989, 0.440433, 0, 0.085271, 0.8125, 0.19375);

        TTokenFactorCalculator fc100(&v, "images", "images", 1.f, 100);
        AssertFactorsEqual(fc100, 0, 0, 0, 0, 0, 0, 0.085271, 0.5, 0.19375);
    }

    void AggrTest() {
        NSc::TValue value = AssertFromJson(
                "[{wizard:{images:{c0:3,sh:9,wn:5,ls:3},video:{c0:2,sh:4,wn:1,ls:3}}},"
                "{wizard:{images:{c0:8,sh:9,wn:1,ls:2}}},{},{},{},{}]");

        TVector<TTokenFactorCalculator> imgCalcs, vidCalcs, blaCalcs;
        for (const auto& v : value.GetArray()) {
            imgCalcs.push_back(TTokenFactorCalculator(&v, "images", ""));
            vidCalcs.push_back(TTokenFactorCalculator(&v, "video", ""));
            blaCalcs.push_back(TTokenFactorCalculator(&v, "blabla", ""));
        }
        auto aggr = [](double& ctr, double& minSp, double& maxSp, double& sumSp, const TVector<TTokenFactorCalculator>& calcers) {
            ctr = 0.f;
            minSp = 0.f;
            maxSp = 0.f;
            sumSp = 0.f;
            for (const auto& c : calcers) {
                ctr = Max(ctr, c.CTR0());
                maxSp = Max(maxSp, c.SurplusToShows());
                minSp = Min(minSp, c.SurplusToShows());
                sumSp += c.SurplusToShows();
            }
        };
        double imgCtr, vidCtr, blaCtr;
        double imgMinSp, vidMinSp, blaMinSp;
        double imgMaxSp, vidMaxSp, blaMaxSp;
        double imgSumSp, vidSumSp, blaSumSp;

        aggr(imgCtr, imgMinSp, imgMaxSp, imgSumSp, imgCalcs);
        aggr(vidCtr, vidMinSp, vidMaxSp, vidSumSp, vidCalcs);
        aggr(blaCtr, blaMinSp, blaMaxSp, blaSumSp, blaCalcs);

        UNIT_ASSERT_DOUBLES_EQUAL(imgCtr, 0.8, 1e-9);
        UNIT_ASSERT_DOUBLES_EQUAL(vidCtr, 0.4, 1e-9);
        UNIT_ASSERT_DOUBLES_EQUAL(blaCtr, 0.0, 1e-9);

        UNIT_ASSERT_DOUBLES_EQUAL(imgMinSp, -0.1, 1e-9);
        UNIT_ASSERT_DOUBLES_EQUAL(vidMinSp, -0.4, 1e-9);
        UNIT_ASSERT_DOUBLES_EQUAL(blaMinSp, 0.0, 1e-9);

        UNIT_ASSERT_DOUBLES_EQUAL(imgMaxSp, 0.2, 1e-9);
        UNIT_ASSERT_DOUBLES_EQUAL(vidMaxSp, 0.0, 1e-9);
        UNIT_ASSERT_DOUBLES_EQUAL(blaMaxSp, 0.0, 1e-9);

        UNIT_ASSERT_DOUBLES_EQUAL(imgSumSp, 0.1, 1e-9);
        UNIT_ASSERT_DOUBLES_EQUAL(vidSumSp, -0.4, 1e-9);
        UNIT_ASSERT_DOUBLES_EQUAL(blaSumSp, 0.0, 1e-9);
    }


private:
    static const TStringBuf Json;
};


const TStringBuf TWizardClicksFactorsTest::Json =
        "{\"navig\":{\"images\":{\"c0\":11,\"c120\":8,\"c15\":10,\"wc\":9.341872442},\"video\":{\"c0\":1,\"c120\":1,\"c15\":1,\"wc\":0.9999999979}},\"pos_wizard\":"
        "{\"images\":{\"0\":{\"c0\":64,\"c120\":55,\"c15\":62,\"wc\":60.45392528,\"csh\":67,\"sh\":117,\"skp\":3,\"ls\":1,\"wn\":62},"
        "\"1\":{\"c0\":5,\"c120\":4,\"c15\":5,\"wc\":4.783909427,\"csh\":5,\"sh\":7,\"wn\":5},\"5\":{\"sh\":1}},"
        "\"video\":{\"0\":{\"csh\":1,\"sh\":1,\"skp\":1,\"ls\":1},\"1\":{\"sh\":1},\"2\":{\"sh\":1},\"3\":{\"sh\":48},\"4\":{\"c0\":1,\"c120\":0,\"c15\":0,\"wc\":0.124826681,\"csh\":1,\"sh\":23},"
        "\"5\":{\"csh\":1,\"sh\":44,\"skp\":1},\"6\":{\"sh\":5}}},\"common\":{\"isr\":31,\"wr\":128,\"wcr0\":6,\"wcr120\":4,\"wcr15\":6,\"wwcr\":5.077919921},"
        "\"wizard\":{\"images\":{\"c0\":69,\"c120\":59,\"c15\":67,\"wc\":65.2378347,\"csh\":72,\"sh\":125,\"skp\":3,\"ls\":1,\"wn\":67},"
        "\"video\":{\"c0\":1,\"c120\":0,\"c15\":0,\"wc\":0.124826681,\"csh\":3,\"sh\":123,\"skp\":2,\"ls\":1}}}";

UNIT_TEST_SUITE_REGISTRATION(TWizardClicksFactorsTest);
