UNITTEST_FOR(kernel/fill_factors_codegen)

OWNER(
    g:base
    sankear
    noobgam
)

SRCS(
    generic_test.cpp
)

PEERDIR(
    kernel/factor_storage
    kernel/generated_factors_info/factors_codegen
    kernel/web_factors_info
    library/cpp/testing/unittest

    search/begemot/rules/query_factors/proto
)

BASE_CODEGEN(kernel/fill_factors_codegen test_fill)

END()
