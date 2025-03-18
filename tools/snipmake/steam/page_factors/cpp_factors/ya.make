LIBRARY()

OWNER(a-bocharov g:snippets)

SRCS(
    annotate_applicator.cpp
    annotate_factors.h
    applicator.cpp
    block_stats.cpp
    common_factors.cpp
    factor_node.cpp
    factor_tree.cpp
    merge_applicator.cpp
    merge_factors.h
    page_segment.cpp
    segmentator.cpp
    split_applicator.cpp
    split_factors.h
    meta_segmentator.cpp
    segmentator_handler.cpp
)

PEERDIR(
    kernel/matrixnet
    library/cpp/domtree
    library/cpp/html/spec
    library/cpp/json
    library/cpp/tokenizer
    library/cpp/wordpos
    library/cpp/numerator
)

BUILD_MN(mxnet_top100/split/matrixnet.info staticMnSplit)
BUILD_MN(mxnet_top100/merge/matrixnet.info staticMnMerge)
BUILD_MN(MULTI mxnet_top100/annotate/matrixnet.mnmc staticMnAnnotate)

END()
