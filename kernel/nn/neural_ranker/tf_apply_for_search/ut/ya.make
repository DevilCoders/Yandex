UNITTEST_FOR(kernel/nn/neural_ranker/tf_apply_for_search)

OWNER(boyalex)

NO_COMPILER_WARNINGS()

PEERDIR(
    library/cpp/testing/unittest
    kernel/nn/neural_ranker/tf_apply_for_search
    kernel/nn/neural_ranker/protos/meta_info
    kernel/nn/neural_ranker/concat_features
)

SIZE(SMALL)

SRCS(
    prod_query_model_ut.cpp
)

DATA(
    sbr://796783527  # query_meta.pb -- query prod model with meta
)

END()

