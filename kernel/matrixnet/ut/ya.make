UNITTEST_FOR(kernel/matrixnet)

FORK_TEST_FILES()
FORK_TESTS()
FORK_SUBTESTS()

OWNER(
    g:base
    ironpeter
    mvel
    apos
)

PEERDIR(
    kernel/relevfml/testmodels
    kernel/matrixnet
    kernel/factor_storage
    kernel/parallel_mx_calcer
    kernel/web_factors_info
    kernel/web_meta_factors_info
    library/cpp/archive
    library/cpp/string_utils/base64
    library/cpp/resource
)

SRCS(
    analysis_ut.cpp
    mn_sse_ut.cpp
    mn_dynamic_ut.cpp
    mn_multi_categ_ut.cpp
    mn_serializer_ut.cpp
)

END()
