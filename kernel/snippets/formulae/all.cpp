#include "all.h"

#include <kernel/snippets/factors/factors.h>

extern const NMatrixnet::TMnSseInfo staticMnActiveLearningBoostTR;
extern const NMatrixnet::TMnSseInfo staticMnRdotsBoostLiteTR;
extern const NMatrixnet::TMnSseInfo staticMnRdotsBoostTR;
extern const NMatrixnet::TMnSseInfo staticMnActiveLearning;
extern const NMatrixnet::TMnSseInfo staticMnActiveLearningBoost;
extern const NMatrixnet::TMnSseInfo staticMnAllMarksBoost;
extern const NMatrixnet::TMnSseInfo staticMnNewMarksPairwiseBoost;
extern const NMatrixnet::TMnSseInfo staticMnNewMarksBoost;
extern const NMatrixnet::TMnSseInfo staticMnRandomWithoutBoost;
extern const NMatrixnet::TMnSseInfo staticMnRandomWithBoost;
extern const NMatrixnet::TMnSseInfo staticMnActiveLearningInformativeness;
extern const NMatrixnet::TMnSseInfo staticMnInformativenessWithReadability;
extern const NMatrixnet::TMnSseInfo staticMnFormula28883;
extern const NMatrixnet::TMnSseInfo staticMnFormula28185;
extern const NMatrixnet::TMnSseInfo staticMnFormula1938;
extern const NMatrixnet::TMnSseInfo staticMnFormula2121;
extern const NMatrixnet::TMnSseInfo staticMnFormula1940;
extern const NMatrixnet::TMnSseInfo staticMnFormula29410;
extern const NMatrixnet::TMnSseInfo staticMnVideo;
extern const NMatrixnet::TMnSseInfo staticMnVideoTr;
extern const NMatrixnet::TMnSseInfo staticMnFormulaSnip1Click10k;
extern const NMatrixnet::TMnSseInfo staticMnFormulaSnip1Click20k;
extern const NMatrixnet::TMnSseInfo staticMnFormulaSnip2Click10k;
extern const NMatrixnet::TMnSseInfo staticMnFormulaSnip2Click20k;
extern const NMatrixnet::TMnSseInfo staticMnFormulaSnip3NoClick10k;
extern const NMatrixnet::TMnSseInfo staticMnFormulaSnip3NoClick20k;

namespace NSnippets {
    const TMxNetFormula ActiveLearningBoostTR("active_learning_boost_tr", algo2Domain, staticMnActiveLearningBoostTR);
    const TMxNetFormula RdotsBoostLiteTR("rdots_boost_lite_tr", algo2Domain, staticMnRdotsBoostLiteTR);
    const TMxNetFormula RdotsBoostTR("rdots_boost_tr", algo2Domain, staticMnRdotsBoostTR);
    const TMxNetFormula ActiveLearning("active_learning", algo2Domain, staticMnActiveLearning);
    const TMxNetFormula ActiveLearningBoost("active_learning_boost", algo2Domain, staticMnActiveLearningBoost);
    const TMxNetFormula AllMxNetPairwiseBoost("all_mpairwise_boost", algo2Domain, staticMnNewMarksPairwiseBoost);
    const TMxNetFormula AllMxNetPairwise("new_marks_boost", algo2Domain, staticMnNewMarksBoost);
    const TMxNetFormula AllMarksBoost("all_marks_boost", algo2Domain, staticMnAllMarksBoost);
    const TMxNetFormula RandomWithoutBoost("random", algo2Domain, staticMnRandomWithoutBoost);
    const TMxNetFormula RandomWithBoost("random_with_boost", algo2Domain, staticMnRandomWithBoost);
    const TMxNetFormula ActiveLearningInformativeness("informativeness", algo2Domain, staticMnActiveLearningInformativeness);
    const TMxNetFormula InformativenessWithReadability("inf_read_formula", algo2Domain, staticMnInformativenessWithReadability);
    const TMxNetFormula AllMxNetFormula28883("formula28883", algo2Domain, staticMnFormula28883);
    const TMxNetFormula AllMxNetFormula28185("formula28185", algo2Domain, staticMnFormula28185);
    const TMxNetFormula AllMxNetFormula1938("formula1938", algo2Domain, staticMnFormula1938);
    const TMxNetFormula AllMxNetFormula2121("formula2121", algo2Domain, staticMnFormula2121);
    const TMxNetFormula AllMxNetFormula1940("formula1940", algo2Domain, staticMnFormula1940);
    const TMxNetFormula AllMxNetFormula29410("formula29410", algo2Domain, staticMnFormula29410);
    const TMxNetFormula AllMxNetVideo("all_video", algo2Domain, staticMnVideo);
    const TMxNetFormula AllMxNetVideoTr("all_video_tr", algo2Domain, staticMnVideoTr);
    const TMxNetFormula AllMxNetFormulaSnip1Click10k("formula_s1c10k", algo2PlusWebV1Domain, staticMnFormulaSnip1Click10k);
    const TMxNetFormula AllMxNetFormulaSnip1Click20k("formula_s1c20k", algo2PlusWebV1Domain, staticMnFormulaSnip1Click20k);
    const TMxNetFormula AllMxNetFormulaSnip2Click10k("formula_s2c10k", algo2PlusWebDomain, staticMnFormulaSnip2Click10k);
    const TMxNetFormula AllMxNetFormulaSnip2Click20k("formula_s2c20k", algo2PlusWebDomain, staticMnFormulaSnip2Click20k);
    const TMxNetFormula AllMxNetFormulaSnip3NoClick10k("formula_s3nc10k", algo2PlusWebNoClickDomain, staticMnFormulaSnip3NoClick10k);
    const TMxNetFormula AllMxNetFormulaSnip3NoClick20k("formula_s3nc20k", algo2PlusWebNoClickDomain, staticMnFormulaSnip3NoClick20k);
}
