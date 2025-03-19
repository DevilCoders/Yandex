#include <kernel/web_factors_info/validators/models_archive_ut_helper.h>
#include <search/formula_chooser/archive_checker/archive_checker.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/memory/blob.h>

namespace {

    using namespace NFormulaChooser;

    // permissions for roles to use certain tags
    THashMap<NProto::EVariable, TVector<NFactor::ETag>> EXCLUSIVE_RULES_FOR_TG_REMOVED {
    };

    THashMap<NProto::EVariable, TVector<NFactor::ETag>> EXCLUSIVE_RULES_FOR_TG_UNIMPLEMENTED {
    };

    /*
    ** Attention!
    ** When you add a new rule to this section,
    ** you allow all future formulas for this role to depend on the tag!
    ** Think about whether the SKIP_MODEL_IDS_ section below would suit you better
    */
    THashMap<NProto::EVariable, TVector<NFactor::ETag>> EXCLUSIVE_RULES_FOR_TG_UNUSED {
        {NProto::V_ALICE_WEB_MUSIC_L3, { NFactor::TG_ALLOW_USE_FOR_ALICE }},
        {NProto::V_DEFAULT_L3,         { NFactor::TG_ALLOW_USE_FOR_PROXIMA_OPT_L3 }},
        {NProto::V_CS_L3,              { NFactor::TG_ALLOW_USE_FOR_PROXIMA_OPT_L3 }},
        {NProto::V_PROXIMA_OPT_L3,     { NFactor::TG_ALLOW_USE_FOR_PROXIMA_OPT_L3 }},
        {NProto::V_PROXIMA_PREDICT,     { NFactor::TG_ALLOW_USE_FOR_PROXIMA_OPT_L3 }},
        {NProto::V_ARABIC_ALICE_WEB_MUSIC_L3, { NFactor::TG_ALLOW_USE_FOR_ALICE }},
        {NProto::V_ARABIC_ALICE_WEB_MUSIC_L2, { NFactor::TG_ALLOW_USE_FOR_ALICE }},
        {NProto::V_SPAM_CONTENT, { NFactor::TG_ALLOW_USE_FOR_SPAM_FORMULAS}},
        {NProto::V_DEFAULT_SPAM_REARR, { NFactor::TG_ALLOW_USE_FOR_SPAM_FORMULAS}},
        {NProto::V_TR_SPAM_REARR, { NFactor::TG_ALLOW_USE_FOR_SPAM_FORMULAS}},
        {NProto::V_SPAM_NAVPARASITES_REARR, { NFactor::TG_ALLOW_USE_FOR_SPAM_FORMULAS}},
    };

    THashMap<NProto::EVariable, TVector<NFactor::ETag>> EXCLUSIVE_RULES_FOR_TG_REUSABLE {
    };


    // formulas with TG_REMOVED factors
    const THashSet<TString> SKIP_MODEL_IDS_REMOVED = {
        "721343", // V_SHMICK_L3 <- (web_production 1566, 1676, 1455)
        "857939", // old antispam BUKI-3178
        "269672", // platinum l1 top (web_production 1566, 1455, 1558, 1447)
        "427587", // tier0 l1 top, (web_production 1566, 1455)
    };

    // formulas with TG_UNIMPLEMENTED factors
    const THashSet<TString> SKIP_MODEL_IDS_UNIMPLEMENTED = {
        "721323", // V_SHMICK_L2 <- (web_production 1862, 1863, 1920)
        "721343", // V_SHMICK_L3 <- (web_production 1862, 1863, 1920)
        "350756"  // V_OLD_L1 <- (web_l1, 120)
    };

    // formulas with TG_UNUSED factors
    const THashSet<TString> SKIP_MODEL_IDS_UNUSED = {
        "779257", // V_BERT_OPT_L3_NEW <- web_meta:544(SplitBertXLLaVMSE)
        "803063", // V_CHOICE_SCREEN_L2 <- web_l2:72(IsConstraintPassed)
        "721323", // V_SHMICK_L2 <- (web_production 30, 450, 451, 1614, 1920)
        "721343",  // V_SHMICK_L3 <- (web_production 30, 450, 451, 653, 1614, 1920)
        "857939",
    };

    // formulas with TG_REUSABLE factors
    const THashSet<TString> SKIP_MODELS_IDS_REUSABLE = {
    };
}

Y_UNIT_TEST_SUITE(FormulasDependencies) {

    Y_UNIT_TEST(DependingOnRemovedFactors) {
        NModelsArchiveValidators::CheckForFactorsByTag(NFactor::TG_REMOVED, EXCLUSIVE_RULES_FOR_TG_REMOVED, SKIP_MODEL_IDS_REMOVED, false);
    }

    Y_UNIT_TEST(DependingOnUnimplementedFactors) {
        NModelsArchiveValidators::CheckForFactorsByTag(NFactor::TG_UNIMPLEMENTED, EXCLUSIVE_RULES_FOR_TG_UNIMPLEMENTED, SKIP_MODEL_IDS_UNIMPLEMENTED, false);
    }

    Y_UNIT_TEST(DependingOnUnusedFactors) {
        NModelsArchiveValidators::CheckForFactorsByTag(NFactor::TG_UNUSED, EXCLUSIVE_RULES_FOR_TG_UNUSED, SKIP_MODEL_IDS_UNUSED, false);
    }

    Y_UNIT_TEST(DependingOnReusableFactors) {
        NModelsArchiveValidators::CheckForFactorsByTag(NFactor::TG_REUSABLE, EXCLUSIVE_RULES_FOR_TG_REUSABLE, SKIP_MODELS_IDS_REUSABLE, false);
    }
}


Y_UNIT_TEST_SUITE(FormulasDependenciesForDefaultMapping) {

    Y_UNIT_TEST(DependingOnRemovedFactors) {
        NModelsArchiveValidators::CheckForFactorsByTag(NFactor::TG_REMOVED, EXCLUSIVE_RULES_FOR_TG_REMOVED, SKIP_MODEL_IDS_REMOVED, true);
    }

    Y_UNIT_TEST(DependingOnUnimplementedFactors) {
        NModelsArchiveValidators::CheckForFactorsByTag(NFactor::TG_UNIMPLEMENTED, EXCLUSIVE_RULES_FOR_TG_UNIMPLEMENTED, SKIP_MODEL_IDS_UNIMPLEMENTED, true);
    }

    Y_UNIT_TEST(DependingOnUnusedFactors) {
        NModelsArchiveValidators::CheckForFactorsByTag(NFactor::TG_UNUSED, EXCLUSIVE_RULES_FOR_TG_UNUSED, SKIP_MODEL_IDS_UNUSED, true);
    }

    Y_UNIT_TEST(DependingOnReusableFactors) {
        NModelsArchiveValidators::CheckForFactorsByTag(NFactor::TG_REUSABLE, EXCLUSIVE_RULES_FOR_TG_REUSABLE, SKIP_MODELS_IDS_REUSABLE, true);
    }
}


Y_UNIT_TEST_SUITE(TestDependenciesOnDeprecated) {
    Y_UNIT_TEST(TestAllFactors) {

        const THashSet<NFactorSlices::EFactorSlice> slicesWillSkip = {
            // ITDITP-1194
            NFactorSlices::EFactorSlice::WEB_ITDITP,
            NFactorSlices::EFactorSlice::WEB_ITDITP_STATIC_FEATURES,
            NFactorSlices::EFactorSlice::WEB_ITDITP_RECOMMENDER,
            NFactorSlices::EFactorSlice::WEB_META_ITDITP
        };

        NModelsArchiveValidators::CheckDependenciesOnDeprecated(slicesWillSkip);
    }
}
