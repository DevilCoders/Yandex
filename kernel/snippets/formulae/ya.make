LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/matrixnet
    kernel/snippets/factors
)

SRCS(
    formula.cpp
    all.cpp
    manual.cpp
    manual_img.cpp
    manual_video.cpp
)

BUILD_MN(InformativenessWithReadability.info staticMnInformativenessWithReadability)

BUILD_MN(ActiveLearningInformativeness.info staticMnActiveLearningInformativeness)

BUILD_MN(Random.info staticMnRandomWithoutBoost)

BUILD_MN(RandomWithBoost.info staticMnRandomWithBoost)

BUILD_MN(ActiveLearningBoostTR.info staticMnActiveLearningBoostTR)

BUILD_MN(RdotsBoostLiteTR.info staticMnRdotsBoostLiteTR)

BUILD_MN(RdotsBoostTR.info staticMnRdotsBoostTR)

BUILD_MN(ActiveLearning.info staticMnActiveLearning)

BUILD_MN(ActiveLearningBoost.info staticMnActiveLearningBoost)

BUILD_MN(AllMarksBoost.info staticMnAllMarksBoost)

BUILD_MN(NewMarksBoost.info staticMnNewMarksBoost)

BUILD_MN(NewMarksPairwiseBoost.info staticMnNewMarksPairwiseBoost)

BUILD_MN(Formula28883.info staticMnFormula28883)

BUILD_MN(Formula28185.info staticMnFormula28185)

BUILD_MN(Formula1938.info staticMnFormula1938)

BUILD_MN(Formula2121.info staticMnFormula2121)

BUILD_MN(Formula1940.info staticMnFormula1940)

BUILD_MN(Formula29410.info staticMnFormula29410)

BUILD_MN(Video.info staticMnVideo)

BUILD_MN(VideoTr.info staticMnVideoTr)

BUILD_MN(Formula77500.info staticMnRegions)

BUILD_MN(snip_1_click_10K_sum_pairwise_boost.info staticMnFormulaSnip1Click10k)

BUILD_MN(snip_1_click_20K_sum_pairwise_boost.info staticMnFormulaSnip1Click20k)

BUILD_MN(snip_2_click_10K_sum_pairwise_boost.info staticMnFormulaSnip2Click10k)

BUILD_MN(snip_2_click_20K_sum_pairwise_boost.info staticMnFormulaSnip2Click20k)

BUILD_MN(snip_3_noclick_10K_sum_pairwise_boost.info staticMnFormulaSnip3NoClick10k)

BUILD_MN(snip_3_noclick_20K_sum_pairwise_boost.info staticMnFormulaSnip3NoClick20k)

END()
