#include "clickint.h"
#include "random.h"

#include <kernel/extended_mx_calcer/factory/factory.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/scheme/scheme_cast.h>

using namespace NExtendedMx;

Y_UNIT_TEST_SUITE(TClickInt) {

    Y_UNIT_TEST(IntersectFeatsTest) {
        NSc::TValue metaScheme;
        TCalcContextMetaProto ctxMeta(&metaScheme);
        *ctxMeta.FeatureContext()["f1"].AvailibleValues().Value__ = NSc::TValue::FromJsonThrow("{q: 1, w:1, e:1}");
        *ctxMeta.FeatureContext()["f2"].AvailibleValues().Value__ = NSc::TValue::FromJsonThrow("{a: 1, s:1, d:1}");
        UNIT_ASSERT(ctxMeta.Validate());

        NSc::TValue additionalScheme;
        TAdditionalFeaturesProto additional(&additionalScheme);
        TAdditionalFeaturesConstProto additionalConst(&additionalScheme);
        // test no intersection with f1
        auto af = additional.Add();
        af.Binary() = false;
        af.Name() = "f1";
        *af.Values().Value__ = NSc::TValue::FromJsonThrow("[a, d]");

        TValidFeatsSet vf;
        IntersectFeats(additionalConst, ctxMeta, vf);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(vf), "{}");

        // test intersection with f2
        af.Name() = "f2";
        IntersectFeats(additionalConst, ctxMeta, vf);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(vf), "{\"f2\":{\"a\":null,\"d\":null}}");

        // add f1 as additional feature
        auto af2 = additional.Add();
        af2.Binary() = false;
        af2.Name() = "f1";
        *af2.Values().Value__ = NSc::TValue::FromJsonThrow("[w]");
        IntersectFeats(additionalConst, ctxMeta, vf);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(vf), "{\"f1\":{\"w\":null},\"f2\":{\"a\":null,\"d\":null}}");
    }

    Y_UNIT_TEST(FillSterAndInsertFeatsWithOneOffsetTest) {
        size_t stepCount = 2;
        size_t totalRows = 4;
        size_t offset = 1;

        auto feats = NJsonConverters::FromJson<TVector<float>>("[1,2,3]");
        TFactors dst;
        FillStepAndInsertFeatsWithOffset(stepCount, totalRows, offset, feats.data(), feats.size(), dst);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(dst), "[[0,1,2,3],[1,1,2,3],[0,1,2,3],[1,1,2,3]]");
    }

    Y_UNIT_TEST(FillSterAndInsertFeatsWithTwoOffestTest) {
        size_t stepCount = 2;
        size_t totalRows = 4;
        size_t offset = 2;

        auto feats = NJsonConverters::FromJson<TVector<float>>("[1,2,3]");
        TFactors dst;
        FillStepAndInsertFeatsWithOffset(stepCount, totalRows, offset, feats.data(), feats.size(), dst);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(dst), "[[0,0,1,2,3],[1,0,1,2,3],[0,0,1,2,3],[1,0,1,2,3]]");
    }

    Y_UNIT_TEST(FillSterAndInsertFeatsWithOffestExceptionsTest) {
        size_t stepCount = 2;
        size_t totalRows = 4;
        size_t offset = 1;

        auto feats = NJsonConverters::FromJson<TVector<float>>("[1,2,3]");
        TFactors dst;

        offset = 0;
        UNIT_ASSERT_EXCEPTION(FillStepAndInsertFeatsWithOffset(stepCount, totalRows, offset, feats.data(), feats.size(), dst), yexception);
        offset = 1;


        totalRows = 5;
        UNIT_ASSERT_EXCEPTION(FillStepAndInsertFeatsWithOffset(stepCount, totalRows, offset, feats.data(), feats.size(), dst), yexception);
        totalRows = 4;
    }

    Y_UNIT_TEST(ExtractAdditionalFeatsResultTest) {
        TCalcContext ctx(false);

        NSc::TValue additionalScheme;
        TAdditionalFeaturesProto additional(&additionalScheme);
        TAdditionalFeaturesConstProto additionalConst(&additionalScheme);
        // test no intersection with f1
        auto af = additional.Add();
        af.Binary() = false;
        af.Name() = "f1";
        *af.Values().Value__ = NSc::TValue::FromJsonThrow("[q,w,e,r,t,y]");

        auto af2 = additional.Add();
        af2.Binary() = true;
        af2.Name() = "f2";
        *af2.Values().Value__ = NSc::TValue::FromJsonThrow("[q,w,e,r,t,y]");
        auto feats = NJsonConverters::FromJson<TVector<float>>("[1, 2, 0,0,0,0,1,0]");
        ExtractAdditionalFeatsResult(nullptr, additionalConst, ctx, feats, true);
        UNIT_ASSERT_STRINGS_EQUAL(ctx.GetResult().FeatureResult()["f1"].Result()->GetString(), "e");
        UNIT_ASSERT_STRINGS_EQUAL(ctx.GetResult().FeatureResult()["f2"].Result()->GetString(), "t");

        // strange factor row
        feats = NJsonConverters::FromJson<TVector<float>>("[1, 100, 0,0,0,0,0,0]");
        ExtractAdditionalFeatsResult(nullptr, additionalConst, ctx, feats, true);
        UNIT_ASSERT_STRINGS_EQUAL(ctx.GetResult().FeatureResult()["f1"].Result()->GetString(), "");
        UNIT_ASSERT_STRINGS_EQUAL(ctx.GetResult().FeatureResult()["f2"].Result()->GetString(), "");
    }

    Y_UNIT_TEST(ReplaceRepeatadlyTest) {
        auto orig = NJsonConverters::FromJson<TFactors>("[[7,7,7,7,7,7,7],[7,7,7,7,7,7,7],[7,7,7,7,7,7,7],[7,7,7,7,7,7,7]]");
        TString etalon =                                 "[[7,7,1,2,7,7,7],[7,7,1,2,7,7,7],[7,7,3,4,7,7,7],[7,7,3,4,7,7,7]]";
        size_t inRowOffset = 2;
        size_t rowsPerFeat = 2;
        auto patch = NJsonConverters::FromJson<TFactors>("[[1,2],[3,4]]");

        ReplaceRepeatadly(orig, patch, inRowOffset, rowsPerFeat);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(orig), etalon);

        etalon =                                        "[[7,7,1,2,7,7,7],[7,7,3,4,7,7,7],[7,7,1,2,7,7,7],[7,7,3,4,7,7,7]]";
        rowsPerFeat = 1;
        ReplaceRepeatadly(orig, patch, inRowOffset, rowsPerFeat);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(orig), etalon);
    }

    Y_UNIT_TEST(ReplaceRepeatadlyTestExceptions) {
        auto orig = NJsonConverters::FromJson<TFactors>("[[7,7,7,7,7,7,7],[7,7,7,7,7,7,7],[7,7,7,7,7,7,7],[7,7,7,7,7,7,7]]");
        size_t inRowOffset = 3;
        size_t rowsPerFeat = 2;
        auto patch = NJsonConverters::FromJson<TFactors>("[[1,2],[3,4]]");

        UNIT_ASSERT_NO_EXCEPTION(ReplaceRepeatadly(orig, patch, inRowOffset, rowsPerFeat));

        rowsPerFeat = 3;
        UNIT_ASSERT_EXCEPTION(ReplaceRepeatadly(orig, patch, inRowOffset, rowsPerFeat), yexception);
        rowsPerFeat = 4;

        patch.clear();
        UNIT_ASSERT_EXCEPTION(ReplaceRepeatadly(orig, patch, inRowOffset, rowsPerFeat), yexception);
    }

    Y_UNIT_TEST(FillPredefinedFeaturesTest) {
        NSc::TValue metaScheme;
        TCalcContextMetaProto ctxMeta(&metaScheme);
        TCalcContext ctx(NSc::TValue::DefaultValue());
        *ctx.GetMeta()->FeatureContext()["f1"].AvailibleValues().Value__ = NSc::TValue::FromJsonThrow("{q: 1, w:1, e:1, r:1}");
        *ctx.GetMeta()->FeatureContext()["f2"].AvailibleValues().Value__ = NSc::TValue::FromJsonThrow("{a: 1, s:1, d:1}");
        UNIT_ASSERT(ctxMeta.Validate());

        NSc::TValue additionalScheme;
        TAdditionalFeaturesProto additional(&additionalScheme);
        TAdditionalFeaturesConstProto additionalConst(&additionalScheme);
        // test no intersection with f1
        auto af = additional.Add();
        af.Binary() = false;
        af.Name() = "f1";
        *af.Values().Value__ = NSc::TValue::FromJsonThrow("[q, w, e, r, t]");

        auto af2 = additional.Add();
        af2.Binary() = true;
        af2.Name() = "f2";
        *af2.Values().Value__ = NSc::TValue::FromJsonThrow("[a, s, d]");


        auto factors = NJsonConverters::FromJson<TVector<float>>("[8, 9]");

        const TString etalon = " \
            [[0,0,1,0,0,8,9],   \
             [1,0,1,0,0,8,9],   \
             [2,0,1,0,0,8,9],   \
             [0,0,0,1,0,8,9],   \
             [1,0,0,1,0,8,9],   \
             [2,0,0,1,0,8,9],   \
             [0,0,0,0,1,8,9],   \
             [1,0,0,0,1,8,9],   \
             [2,0,0,0,1,8,9],   \
             [0,1,1,0,0,8,9],   \
             [1,1,1,0,0,8,9],   \
             [2,1,1,0,0,8,9],   \
             [0,1,0,1,0,8,9],   \
             [1,1,0,1,0,8,9],   \
             [2,1,0,1,0,8,9],   \
             [0,1,0,0,1,8,9],   \
             [1,1,0,0,1,8,9],   \
             [2,1,0,0,1,8,9],   \
             [0,2,1,0,0,8,9],   \
             [1,2,1,0,0,8,9],   \
             [2,2,1,0,0,8,9],   \
             [0,2,0,1,0,8,9],   \
             [1,2,0,1,0,8,9],   \
             [2,2,0,1,0,8,9],   \
             [0,2,0,0,1,8,9],   \
             [1,2,0,0,1,8,9],   \
             [2,2,0,0,1,8,9],   \
             [0,3,1,0,0,8,9],   \
             [1,3,1,0,0,8,9],   \
             [2,3,1,0,0,8,9],   \
             [0,3,0,1,0,8,9],   \
             [1,3,0,1,0,8,9],   \
             [2,3,0,1,0,8,9],   \
             [0,3,0,0,1,8,9],   \
             [1,3,0,0,1,8,9],   \
             [2,3,0,0,1,8,9]]" ;
        TFactors res;
        TDebug dbg(false);
        FillPredefinedFeatures(nullptr, res, factors.data(), factors.size(), 3, additionalConst, ctx);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(res), NSc::TValue::FromJsonThrow(etalon).ToJson());
    }

    Y_UNIT_TEST(FillPredefinedFeaturesNoStepTest) {
        TCalcContext ctx(NSc::TValue::DefaultValue());
        *ctx.GetMeta()->FeatureContext()["f1"].AvailibleValues().Value__ = NSc::TValue::FromJsonThrow("{q: 1, w:1, e:1, r:1}");
        *ctx.GetMeta()->FeatureContext()["f2"].AvailibleValues().Value__ = NSc::TValue::FromJsonThrow("{a: 1, s:1, d:1}");
        UNIT_ASSERT(ctx.GetMeta().Validate());

        NSc::TValue additionalScheme;
        TAdditionalFeaturesProto additional(&additionalScheme);
        TAdditionalFeaturesConstProto additionalConst(&additionalScheme);
        // test no intersection with f1
        auto af = additional.Add();
        af.Binary() = false;
        af.Name() = "f1";
        *af.Values().Value__ = NSc::TValue::FromJsonThrow("[q, w, e, r, t]");

        auto af2 = additional.Add();
        af2.Binary() = true;
        af2.Name() = "f2";
        *af2.Values().Value__ = NSc::TValue::FromJsonThrow("[a, s, d]");


        auto factors = NJsonConverters::FromJson<TVector<float>>("[8, 9]");

        const TString etalon = " \
           [[0,1,0,0,8,9],  \
            [0,0,1,0,8,9],  \
            [0,0,0,1,8,9],  \
            [1,1,0,0,8,9],  \
            [1,0,1,0,8,9],  \
            [1,0,0,1,8,9],  \
            [2,1,0,0,8,9],  \
            [2,0,1,0,8,9],  \
            [2,0,0,1,8,9],  \
            [3,1,0,0,8,9],  \
            [3,0,1,0,8,9],  \
            [3,0,0,1,8,9]]";
        TFactors res;
        TDebug dbg(false);
        FillPredefinedFeatures(nullptr, res, factors.data(), factors.size(), NO_STEP, additionalConst, ctx);
        UNIT_ASSERT_STRINGS_EQUAL(NJsonConverters::ToJson(res), NSc::TValue::FromJsonThrow(etalon).ToJson());
    }
}

Y_UNIT_TEST_SUITE(TRandomIdx) {

    TExtendedLoaderFactory Factory;
    const TVector<float> Factors;

    Y_UNIT_TEST(TGoodInitTest) {
        TStringStream ss = TString(R"({ "XtdType": "idx_random", "Params": { "Probabilities": [ 0.03, 0.05, 0.10] } })");
        UNIT_ASSERT_NO_EXCEPTION(TCalcerPtr(Factory.Load(&ss)));
    }

    Y_UNIT_TEST(TBadInitTest) {
        // negative
        TStringStream ss = TString(R"({ "XtdType": "idx_random", "Params": { "Probabilities": [ 0.03, -0.05, 0.10] } })");
        UNIT_ASSERT_EXCEPTION(Factory.Load(&ss), yexception);

        // bad idx2pos
        ss = TString(R"({ "XtdType": "idx_random", "Params": { "Probabilities": [ 0.03, -0.05, 0.10], "Idx2Pos": [0] } })");
        UNIT_ASSERT_EXCEPTION(Factory.Load(&ss), yexception);

        // zero sum
        ss = TString(R"({ "XtdType": "idx_random", "Params": { "Probabilities": [ 0.00, 0.00, 0.00] } })");
        UNIT_ASSERT_EXCEPTION(Factory.Load(&ss), yexception);

        // empty
        ss = TString(R"({ "XtdType": "idx_random", "Params": { "Probabilities": [] } })");
        UNIT_ASSERT_EXCEPTION(Factory.Load(&ss), yexception);
    }

    Y_UNIT_TEST(TGoodByPosInitTest) {
        TStringStream ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "Probabilities": [ 0.03, 0.05, 0.10],
                "ProbabilitiesByPos" : [
                    [0.03, 0.05, 0.10],
                    [0.05, 0.10, 0.15],
                ]
            }
        })");
        UNIT_ASSERT_NO_EXCEPTION(TCalcerPtr(Factory.Load(&ss)));
    }

    Y_UNIT_TEST(TBadByPosInitTest) {
        //negative
        TStringStream ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "Probabilities": [ 0.03, 0.05, 0.10],
                "ProbabilitiesByPos" : [
                    [0.03, -0.05, 0.10],
                    [0.05, 0.10, 0.15],
                ]
            }
        })");
        UNIT_ASSERT_EXCEPTION(Factory.Load(&ss), yexception);

        // zero sum
        ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "Probabilities": [ 0.03, 0.05, 0.10],
                "ProbabilitiesByPos" : [
                    [0.03, -0.05, 0.10],
                    [0.05, 0.05, 0.00],
                ]
            }
        })");
        UNIT_ASSERT_EXCEPTION(Factory.Load(&ss), yexception);

        // empty
        ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "Probabilities": [ 0.03, 0.05, 0.10],
                "ProbabilitiesByPos" : [
                    [0.03, 0.05, 0.10],
                    [],
                ]
            }
        })");
        UNIT_ASSERT_EXCEPTION(Factory.Load(&ss), yexception);
    }

    Y_UNIT_TEST(TCalculateSelectSumsWithAbsentPosTest) {
        TStringStream ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "FeatureName": "Pos",
                "Probabilities": [ 0.01, 0.01, 0.01],
                "ProbabilitiesByPos" : [
                    [0.01, 0.02, 0.04],
                    [0.01, 0.03, 0.09],
                ]
            }
        })");

        TCalcerPtr calcer = THolder(Factory.Load(&ss));
        NExtendedMx::TCalcContext ctx(true);
        calcer->CalcRelevExtended(Factors, ctx);
        const TString& etalonCtx = R"({"idx_random":{"feature_Pos":1,"w_feature_Pos":0.3333333333}})";
        const TString& etalonDbgLog = R"(clickint random calculation started: idx_random
seed:
do not show idx: -1
may dont show: 0
selected partial sums: 0.01 0.02 0.03
)";
        UNIT_ASSERT_VALUES_EQUAL(ctx.GetLog().ToJson(), etalonCtx);
        UNIT_ASSERT_VALUES_EQUAL(ctx.DbgLog().ToString(), etalonDbgLog);
    }

    Y_UNIT_TEST(TCalculateSelectSumsWithPosTest) {
        TStringStream ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "FeatureName": "Pos",
                "Probabilities": [ 0.01, 0.01, 0.01],
                "ProbabilitiesByPos" : [
                    [0.01, 0.02, 0.04],
                    [0.01, 0.03, 0.09],
                ]
            }
        })");

        TCalcerPtr calcer = THolder(Factory.Load(&ss));
        NExtendedMx::TCalcContext ctx(true);
        ctx.GetMeta().PredictedPosInfo() = 1;
        calcer->CalcRelevExtended(Factors, ctx);
        const TString& etalonCtx = R"({"idx_random":{"feature_Pos":2,"w_feature_Pos":0.6923076923}})";
        const TString& etalonDbgLog = R"(clickint random calculation started: idx_random
seed:
do not show idx: -1
may dont show: 0
selected partial sums: 0.01 0.04 0.13
)";
        UNIT_ASSERT_VALUES_EQUAL(ctx.GetLog().ToJson(), etalonCtx);
        UNIT_ASSERT_VALUES_EQUAL(ctx.DbgLog().ToString(), etalonDbgLog);
    }

    Y_UNIT_TEST(TCalculateSelectSumsWithTooBigPosTest) {
        TStringStream ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "FeatureName": "Pos",
                "Probabilities": [ 0.01, 0.01, 0.01],
                "ProbabilitiesByPos" : [
                    [0.01, 0.02, 0.04],
                    [0.01, 0.03, 0.09],
                ]
            }
        })");

        TCalcerPtr calcer = THolder(Factory.Load(&ss));
        NExtendedMx::TCalcContext ctx(true);
        ctx.GetMeta().PredictedPosInfo() = 100;
        calcer->CalcRelevExtended(Factors, ctx);
        const TString& etalonCtx = R"({"idx_random":{"feature_Pos":1,"w_feature_Pos":0.3333333333}})";
        const TString& etalonDbgLog = R"(clickint random calculation started: idx_random
seed:
do not show idx: -1
may dont show: 0
selected partial sums: 0.01 0.02 0.03
)";
        UNIT_ASSERT_VALUES_EQUAL(ctx.GetLog().ToJson(), etalonCtx);
        UNIT_ASSERT_VALUES_EQUAL(ctx.DbgLog().ToString(), etalonDbgLog);
    }

    Y_UNIT_TEST(TCalculateSelectSumsWithPosAndNoShowTest) {
        TStringStream ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "FeatureName": "Pos",
                "Probabilities": [ 0.01, 0.01, 0.01],
                "ProbabilitiesByPos" : [
                    [0.01, 0.02, 0.04],
                    [0.01, 0.03, 0.09],
                ],
                "Idx2Pos": [100, 77, 88]
            }
        })");
        TCalcerPtr calcer = THolder(Factory.Load(&ss));
        NExtendedMx::TCalcContext ctx(true);
        ctx.GetMeta().PredictedPosInfo() = 100;
        calcer->CalcRelevExtended(Factors, ctx);
        const TString& etalonCtx = R"({"idx_random":{"feature_Pos":77,"feature_noshow_p":0.3333333333,"mbskipped":1,"w_feature_Pos":0.3333333333}})";
        const TString& etalonDbgLog = R"(clickint random calculation started: idx_random
seed:
do not show idx: 0
may dont show: 1
selected partial sums: 0.01 0.02 0.03
)";
        UNIT_ASSERT_VALUES_EQUAL(ctx.GetLog().ToJson(), etalonCtx);
        UNIT_ASSERT_VALUES_EQUAL(ctx.DbgLog().ToString(), etalonDbgLog);
    }

    Y_UNIT_TEST(TCalculateSelectSumsWithPosAndSkipNoShowTest) {
        TStringStream ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "FeatureName": "Pos",
                "Probabilities": [ 0., 0.01, 0.01],
                "ProbabilitiesByPos" : [
                    [0.01, 0.02, 0.04],
                    [0.01, 0.03, 0.09],
                ],
                "Idx2Pos": [100, 77, 88]
            }
        })");
        TCalcerPtr calcer = THolder(Factory.Load(&ss));
        NExtendedMx::TCalcContext ctx(true);
        ctx.GetMeta().PredictedPosInfo() = 100;
        calcer->CalcRelevExtended(Factors, ctx);
        const TString& etalonCtx = R"({"idx_random":{"feature_Pos":88,"w_feature_Pos":0.5}})";
        const TString& etalonDbgLog = R"(clickint random calculation started: idx_random
seed:
do not show idx: 0
may dont show: 0
selected partial sums: 0 0.01 0.02
)";
        UNIT_ASSERT_VALUES_EQUAL(ctx.GetLog().ToJson(), etalonCtx);
        UNIT_ASSERT_VALUES_EQUAL(ctx.DbgLog().ToString(), etalonDbgLog);
    }
    Y_UNIT_TEST(TCalculateSelectSumsWithShowProbaMultiplierIfFilteredTest) {
        TStringStream ss = TString(R"({
            "XtdType": "idx_random",
            "Params": {
                "FeatureName": "Pos",
                "Probabilities": [ 0.01, 0.02, 0.02],
                "ProbabilitiesByPos" : [
                    [0.01, 0.02, 0.04],
                    [0.01, 0.03, 0.09],
                ],
                "Idx2Pos": [100, 77, 88],
                "ShowProbaMultiplierIfFiltered": 0.5,
            }
        })");
        TCalcerPtr calcer = THolder(Factory.Load(&ss));
        NExtendedMx::TCalcContext ctx(true);
        ctx.GetMeta().PredictedPosInfo() = 100;
        ctx.GetMeta().PredictedFeaturesInfo().IsFiltered() = true;
        calcer->CalcRelevExtended(Factors, ctx);
        const TString& etalonCtx = R"({"idx_random":{"feature_Pos":100,"feature_noshow_p":0.6,"mbskipped":1,"w_feature_Pos":0.6}})";
        const TString& etalonDbgLog = R"(clickint random calculation started: idx_random
seed:
do not show idx: 0
may dont show: 1
selected partial sums: 0.06 0.08 0.1
)";
        UNIT_ASSERT_VALUES_EQUAL(ctx.GetLog().ToJson(), etalonCtx);
        UNIT_ASSERT_VALUES_EQUAL(ctx.DbgLog().ToString(), etalonDbgLog);
    }
}
