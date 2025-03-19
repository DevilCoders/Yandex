#include "features_remap.h"
#include <library/cpp/testing/unittest/registar.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/web_l2_factors_info/factor_names.h>

using namespace NFeaturesRemap::NProto;

Y_UNIT_TEST_SUITE(Calculation) {
    TString MainProgramExample = R"(
        StageCalculation {
            Stage: BEFORE_NN_OVER_FEATURES
            Let { DefineVar: "Var1"; FeatureSrc { Slice: "web_l2"; Name: "TitleWordCoverageForm" } }
            Let { DefineVar: "Var2"; FeatureSrc { Slice: "web_l2"; Id: 6 } }
            Set { Expression: "(Var1 + Var2) / 2"; FeatureDst { Slice: "web_production"; Id: 4 } }
            Set { Expression: "0.3"; FeatureDst { Slice: "web_production"; Id: 1 } }
            Set { Expression: "Z"; FeatureDst { Slice: "web_l2"; Name: "TitleWordCoverageForm" } }
        }
    )";

    TString ComplexProgramExample = R"(
        StageCalculation {
            Stage: BEFORE_NN_OVER_FEATURES
            Let { DefineVar: "V0"; FeatureSrc { Slice: "web_production"; Id: 1354} }
            Let { DefineVar: "V1"; FeatureSrc { Slice: "web_production"; Id: 1489} }
            Let { DefineVar: "V2"; FeatureSrc { Slice: "web_production"; Id: 1576} }
            Let { DefineVar: "V3"; FeatureSrc { Slice: "web_production"; Id: 1588} }
            Let { DefineVar: "V4"; FeatureSrc { Slice: "web_production"; Id: 1597} }
            Let { DefineVar: "V5"; FeatureSrc { Slice: "web_production"; Id: 1598} }
            Let { DefineVar: "V6"; FeatureSrc { Slice: "web_production"; Id: 1773} }
            Let { DefineVar: "V7"; FeatureSrc { Slice: "web_production"; Id: 1849} }
            Let { DefineVar: "V8"; FeatureSrc { Slice: "web_production"; Id: 1850} }
            Let { DefineVar: "V9"; FeatureSrc { Slice: "web_production"; Id: 1855} }
            Let { DefineVar: "V10"; FeatureSrc { Slice: "web_production"; Id: 1885} }
            Let { DefineVar: "V11"; FeatureSrc { Slice: "web_production"; Id: 1892} }
            Let { DefineVar: "V12"; FeatureSrc { Slice: "web_production"; Id: 1898} }
            Set { Expression: "4.6185*V6+0.6594*V1-0.6371*#SQRT#(V4)+0.1057*V12+0.1005*V11+0.0732*#SQRT#(V8)+0.0276*V10+0.0593*V9+0.0318*#SQRT#(V3)+0.0015*#SQRT#(V7)-2.7521"; FeatureDst { Slice: "web_production"; Id: 1338 } }
            Set { Expression: "0.6327"; FeatureDst { Slice: "web_production"; Id: 1504 } }
            Set { Expression: "0.8203*#SQRT#(V0)-0.0167*V7+0.0176*#SQRT#(V8)+0.0147"; FeatureDst { Slice: "web_production"; Id: 1472 } }
            Set { Expression: "0.6298"; FeatureDst { Slice: "web_production"; Id: 1405 } }
            Set { Expression: "0.322*V9+0.0106*V2+0.0314*#SQRT#(V5)-0.0475*V12+0.4759"; FeatureDst { Slice: "web_production"; Id: 1406 } }
        }
    )";

    TFactorBorders GetBorders() {
        TFactorBorders borders;
        DeserializeFactorBorders("web_production[0;10) web_l2[10;20)", borders);
        NFactorSlices::NDetail::EnsuredReConstruct(borders);
        return borders;
    }

    Y_UNIT_TEST(Parsing) {
        TCalculations program = NFeaturesRemap::ParseTxt(MainProgramExample);
        NFeaturesRemap::ClearHrInfo(program);
    }

    Y_UNIT_TEST(ParsingComplex) {
        TCalculations program = NFeaturesRemap::ParseTxt(ComplexProgramExample);
        NFeaturesRemap::ClearHrInfo(program);
    }

    Y_UNIT_TEST(MainScenario) {
        TCalculations program = NFeaturesRemap::ParseTxt(MainProgramExample);
        NFeaturesRemap::ClearHrInfo(program);

        TFactorStorage storage{GetBorders()};
        auto view = storage.CreateView();
        view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM] = 0.5;
        view[10 + 6] = 0.7;
        view[4] = 0;
        auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "", {{"Z", 0.1}});
        UNIT_ASSERT(!result.HasErrors);
        UNIT_ASSERT(result.DoneModifications);
        UNIT_ASSERT_DOUBLES_EQUAL(view[4], 0.6, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(view[1], 0.3, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM], 0.1, 1e-5);
    }

    Y_UNIT_TEST(MainScenarioPackUnpack) {
        TCalculations programBefore = NFeaturesRemap::ParseTxt(MainProgramExample);
        TCalculations program = NFeaturesRemap::ParseCgi(NFeaturesRemap::ToCgiAutoConvertations(programBefore, false), true);

        TFactorStorage storage{GetBorders()};
        auto view = storage.CreateView();
        view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM] = 0.5;
        view[10 + 6] = 0.7;
        view[4] = 0;
        auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "", {{"Z", 0.1}});
        UNIT_ASSERT(!result.HasErrors);
        UNIT_ASSERT(result.DoneModifications);
        UNIT_ASSERT_DOUBLES_EQUAL(view[4], 0.6, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(view[1], 0.3, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM], 0.1, 1e-5);
    }

    Y_UNIT_TEST(ComplexProgramPackUnpackWithPronParsing) {
        TCalculations programBefore = NFeaturesRemap::ParseTxt(ComplexProgramExample);
        TString cgi = "&pron=features_remap%3DCuYFCAMSGQoCVjASEwoOd2ViX3Byb2R1Y3Rpb24YygoSGQoCVjESEwoOd2ViX3Byb2R1Y3Rp"
                      "b24Y0QsSGQoCVjISEwoOd2ViX3Byb2R1Y3Rpb24YqAwSGQoCVjMSEwoOd2ViX3Byb2R1Y3Rpb24YtAwSGQoCVjQSEwoOd2V"
                      "iX3Byb2R1Y3Rpb24YvQwSGQoCVjUSEwoOd2ViX3Byb2R1Y3Rpb24YvgwSGQoCVjYSEwoOd2ViX3Byb2R1Y3Rpb24Y7Q0SGQ"
                      "oCVjcSEwoOd2ViX3Byb2R1Y3Rpb24YuQ4SGQoCVjgSEwoOd2ViX3Byb2R1Y3Rpb24Yug4SGQoCVjkSEwoOd2ViX3Byb2R1Y"
                      "3Rpb24Yvw4SGgoDVjEwEhMKDndlYl9wcm9kdWN0aW9uGN0OEhoKA1YxMRITCg53ZWJfcHJvZHVjdGlvbhjkDhIaCgNWMTIS"
                      "EwoOd2ViX3Byb2R1Y3Rpb24Y6g4apQEKjQE0LjYxODUqVjYrMC42NTk0KlYxLTAuNjM3MSojU1FSVCMoVjQpKzAuMTA1Nyp"
                      "WMTIrMC4xMDA1KlYxMSswLjA3MzIqI1NRUlQjKFY4KSswLjAyNzYqVjEwKzAuMDU5MypWOSswLjAzMTgqI1NRUlQjKFYzKS"
                      "swLjAwMTUqI1NRUlQjKFY3KS0yLjc1MjESEwoOd2ViX3Byb2R1Y3Rpb24YugoaHQoGMC42MzI3EhMKDndlYl9wcm9kdWN0a"
                      "W9uGOALGksKNDAuODIwMyojU1FSVCMoVjApLTAuMDE2NypWNyswLjAxNzYqI1NRUlQjKFY4KSswLjAxNDcSEwoOd2ViX3By"
                      "b2R1Y3Rpb24YwAsaHQoGMC42Mjk4EhMKDndlYl9wcm9kdWN0aW9uGP0KGk0KNjAuMzIyKlY5KzAuMDEwNipWMiswLjAzMTQ"
                      "qI1NRUlQjKFY1KS0wLjA0NzUqVjEyKzAuNDc1ORITCg53ZWJfcHJvZHVjdGlvbhj%2BCg%3D%3D";
        TString cgi2 = "&pron=features_remap%3D" + NFeaturesRemap::ToCgiAutoConvertations(programBefore, true);
        UNIT_ASSERT(cgi == cgi2);
        TCalculations program = NFeaturesRemap::ParseCgi(NFeaturesRemap::ToCgiAutoConvertations(programBefore, true), false);
        NFeaturesRemap::RestoreFullInfo(programBefore);
        NFeaturesRemap::RestoreFullInfo(program);
        UNIT_ASSERT(programBefore.GetStageCalculation().size() == program.GetStageCalculation().size());
        for (int stageInd = 0; stageInd < programBefore.GetStageCalculation().size(); stageInd++) {
            const auto& beforeStage = programBefore.GetStageCalculation()[stageInd];
            const auto& afterStage = program.GetStageCalculation()[stageInd];
            UNIT_ASSERT(beforeStage.GetLet().size() == afterStage.GetLet().size());
            UNIT_ASSERT(beforeStage.GetSet().size() == afterStage.GetSet().size());
            for (int i = 0; i < beforeStage.GetLet().size(); i++) {
                const auto& beforeLet = beforeStage.GetLet()[i];
                const auto& afterLet = afterStage.GetLet()[i];
                UNIT_ASSERT(beforeLet.GetFeatureSrc().GetId() == afterLet.GetFeatureSrc().GetId());
                UNIT_ASSERT(beforeLet.GetFeatureSrc().GetSlice() == afterLet.GetFeatureSrc().GetSlice());
            }
            for (int i = 0; i < beforeStage.GetSet().size(); i++) {
                const auto& beforeSet = beforeStage.GetSet()[i];
                const auto& afterSet = afterStage.GetSet()[i];
                UNIT_ASSERT(beforeSet.GetFeatureDst().GetId() == afterSet.GetFeatureDst().GetId());
                UNIT_ASSERT(beforeSet.GetFeatureDst().GetSlice() == afterSet.GetFeatureDst().GetSlice());
            }
        }
    }

    Y_UNIT_TEST(CheckWithSearchTypesPositive) {
        TCalculations program = NFeaturesRemap::ParseTxt(MainProgramExample);
        NFeaturesRemap::ClearHrInfo(program);

        TFactorStorage storage{GetBorders()};
        auto view = storage.CreateView();
        view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM] = 0.5;
        view[10 + 6] = 0.7;
        view[4] = 0;
        auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "web_mmeta", {{"Z", 0.1}});
        UNIT_ASSERT(!result.HasErrors);
        UNIT_ASSERT(result.DoneModifications);
    }

    Y_UNIT_TEST(CheckWithSearchTypes2) {
        TCalculations program = NFeaturesRemap::ParseTxt(MainProgramExample);
        program.MutableStageCalculation(0)->AddSearchTypes("web_mmeta");
        program.MutableStageCalculation(0)->AddSearchTypes("web");
        NFeaturesRemap::ClearHrInfo(program);

        TFactorStorage storage{GetBorders()};
        auto view = storage.CreateView();
        view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM] = 0.5;
        view[10 + 6] = 0.7;
        view[4] = 0;
        {
            auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "web_mmeta", {{"Z", 0.1}});
            UNIT_ASSERT(!result.HasErrors);
            UNIT_ASSERT(result.DoneModifications);
        }
        {
            auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "xxx", {{"Z", 0.1}});
            UNIT_ASSERT(!result.HasErrors);
            UNIT_ASSERT(!result.DoneModifications);
        }
        {
            auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "", {{"Z", 0.1}});
            UNIT_ASSERT(!result.HasErrors);
            UNIT_ASSERT(result.DoneModifications);
        }
    }

    Y_UNIT_TEST(CornerCaseLet) {
        TCalculations program = NFeaturesRemap::ParseTxt(MainProgramExample);
        NFeaturesRemap::ClearHrInfo(program);
        TFactorStorage storage{GetBorders()};
        auto view = storage.CreateView();
        view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM] = 0.5;
        view[10 + 6] = 0.7;
        view[4] = 0;

        NFeaturesRemap::TCalcApplyReport result;
        for(auto i : xrange(10, 100)) {
            program.MutableStageCalculation(0)->MutableLet(1)->MutableFeatureSrc()->SetId(i);
            result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "", {{"Z", 0.1}});
            UNIT_ASSERT(result.HasErrors);
        }
        Cerr << result.ErrorsReport << Endl;
    }

    Y_UNIT_TEST(CornerCaseSet) {
        TCalculations program = NFeaturesRemap::ParseTxt(MainProgramExample);
        NFeaturesRemap::ClearHrInfo(program);
        TFactorStorage storage{GetBorders()};
        auto view = storage.CreateView();
        view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM] = 0.5;
        view[10 + 6] = 0.7;
        view[4] = 0;

        NFeaturesRemap::TCalcApplyReport result;
        for(auto i : xrange(10, 100)) {
            program.MutableStageCalculation(0)->MutableSet(1)->MutableFeatureDst()->SetId(i);
            result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "", {{"Z", 0.1}});
            UNIT_ASSERT(result.HasErrors);
        }
        Cerr << result.ErrorsReport << Endl;
    }

    Y_UNIT_TEST(CornerCaseSlice) {
        TCalculations program = NFeaturesRemap::ParseTxt(MainProgramExample);
        NFeaturesRemap::ClearHrInfo(program);
        TFactorStorage storage{GetBorders()};
        auto view = storage.CreateView();
        view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM] = 0.5;
        view[10 + 6] = 0.7;
        view[4] = 0;

        program.MutableStageCalculation(0)->MutableSet(1)->MutableFeatureDst()->SetSlice("web_meta");
        auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "", {});
        UNIT_ASSERT(result.HasErrors);
        Cerr << result.ErrorsReport << Endl;
    }

    Y_UNIT_TEST(CornerCaseSlice2) {
        TCalculations program = NFeaturesRemap::ParseTxt(MainProgramExample);
        NFeaturesRemap::ClearHrInfo(program);
        TFactorStorage storage{GetBorders()};
        auto view = storage.CreateView();
        view[10 + NSliceWebL2::FI_TITLE_WORD_COVERAGE_FORM] = 0.5;
        view[10 + 6] = 0.7;
        view[4] = 0;

        program.MutableStageCalculation(0)->MutableSet(1)->MutableFeatureDst()->SetSlice("xxx");
        auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "", {});
        UNIT_ASSERT(result.HasErrors);
        Cerr << result.ErrorsReport << Endl;
    }

    Y_UNIT_TEST(RepeatedIfTest) {
        TStringBuf programWithRepeatedIf = R"a(
            StageCalculation {
                Stage: BEFORE_NN_OVER_FEATURES
                Let { DefineVar: "V0"; FeatureSrc { Slice: "web_production"; Id: 0} }
                Let { DefineVar: "V1"; FeatureSrc { Slice: "web_production"; Id: 1} }
                Let { DefineVar: "V2"; FeatureSrc { Slice: "web_production"; Id: 2} }
                If {
                    Condition: "V0 > 5 || V1 == 0";
                    Set { Expression: "V0 + V1"; FeatureDst { Slice: "web_production"; Id: 3 } }
                }
                If {
                    Condition: "V1 != 0 && V2 < 2";
                    Set { Expression: "V0 + (V1 || V2)"; FeatureDst { Slice: "web_production"; Id: 3 } }
                }
            }
        )a";
        TCalculations program = NFeaturesRemap::ParseTxt(programWithRepeatedIf);
        TFactorStorage storage{GetBorders()};
        storage[{ EFactorSlice::WEB_PRODUCTION, 0}] = 5;
        storage[{ EFactorSlice::WEB_PRODUCTION, 1}] = 2;
        storage[{ EFactorSlice::WEB_PRODUCTION, 2}] = 1;
        auto view = storage.CreateView();
        auto result = NFeaturesRemap::DoCalculation(program, view, EStage::BEFORE_NN_OVER_FEATURES, "", {});

        UNIT_ASSERT(result.DoneModifications);
        NFactorSlices::TFullFactorIndex resultIndex { EFactorSlice::WEB_PRODUCTION, 3 };
        UNIT_ASSERT_DOUBLES_EQUAL(view[resultIndex], 6, 0);
    }

    Y_UNIT_TEST(OneDocOptimation) {
        TStringBuf programWithRepeatedIf = R"a(
            StageCalculation {
                Stage: BEFORE_NN_OVER_FEATURES
                Let { DefineVar: "V0"; FeatureSrc { Slice: "web_production"; Id: 0} }
                Let { DefineVar: "V1"; FeatureSrc { Slice: "web_production"; Id: 1} }
                Let { DefineVar: "V2"; FeatureSrc { Slice: "web_production"; Id: 2} }
                Set { Expression: "V0 + V1"; FeatureDst { Slice: "web_production"; Id: 3 } }
            }
            AssumeThatAllVariablesAreConstantOverDocs: true
        )a";
        TCalculations program = NFeaturesRemap::ParseTxt(programWithRepeatedIf);
        TVector<TFactorStorage> storages(10, TFactorStorage{GetBorders()});
        TVector<TFactorStorage*> storagePtrs;
        for (auto& storage : storages) {
            storage[{ EFactorSlice::WEB_PRODUCTION, 0}] = 5;
            storage[{ EFactorSlice::WEB_PRODUCTION, 1}] = 2;
            storage[{ EFactorSlice::WEB_PRODUCTION, 2}] = 1;
            storagePtrs.push_back(&storage);
        }
        auto result = NFeaturesRemap::DoCalculation(program, storagePtrs, EStage::BEFORE_NN_OVER_FEATURES, "", {});
        UNIT_ASSERT(result.DoneModifications);
        UNIT_ASSERT(!result.HasErrors);
        NFactorSlices::TFullFactorIndex resultIndex { EFactorSlice::WEB_PRODUCTION, 3 };
        for (const auto& storage : storages) {
            UNIT_ASSERT_DOUBLES_EQUAL(storage[resultIndex], 7, 0);
        }
    }
}
