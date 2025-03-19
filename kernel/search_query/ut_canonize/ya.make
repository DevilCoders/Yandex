EXECTEST()

SIZE(MEDIUM)

OWNER(
    g:factordev
    timuratshin
)

RUN(
    NAME CanonizeCNorm
    ENV OutRes=1
    kernel-search_query-ut TConfigurableNormalizerTest::CanonizeCNorm
    STDOUT ${TEST_OUT_ROOT}/test_with_cnorm.out
    CANONIZE ${TEST_OUT_ROOT}/test_with_cnorm.out
)

RUN(
    NAME CanonizeBNorm
    ENV OutRes=1
    kernel-search_query-ut TConfigurableNormalizerTest::CanonizeBNorm
    STDOUT ${TEST_OUT_ROOT}/test_with_bnorm.out
    CANONIZE ${TEST_OUT_ROOT}/test_with_bnorm.out
)

RUN(
    NAME CanonizeBNormOld
    ENV OutRes=1
    kernel-search_query-ut TConfigurableNormalizerTest::CanonizeBNormOld
    STDOUT ${TEST_OUT_ROOT}/test_with_bnorm_old.out
    CANONIZE ${TEST_OUT_ROOT}/test_with_bnorm_old.out
)

RUN(
    NAME CanonizeANorm
    ENV OutRes=1
    kernel-search_query-ut TConfigurableNormalizerTest::CanonizeANorm
    STDOUT ${TEST_OUT_ROOT}/test_with_anorm.out
    CANONIZE ${TEST_OUT_ROOT}/test_with_anorm.out
)

DEPENDS(
    kernel/search_query/ut
)

END()
