EXECTEST()

OWNER(
    setser
)

RUN(
    NAME TestHead
    STDOUT ${TEST_CASE_ROOT}/stdout
    library-cpp-libgit2_wrapper-ut TRepositoryTests::TestHead
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/stdout
)

RUN(
    NAME TestCheckout
    STDOUT ${TEST_CASE_ROOT}/stdout
    library-cpp-libgit2_wrapper-ut TRepositoryTests::TestCheckout
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/stdout
)

RUN(
    NAME TestForEachFile
    STDOUT ${TEST_CASE_ROOT}/stdout
    library-cpp-libgit2_wrapper-ut TRepositoryTests::TestForEachFile
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/stdout
)

RUN(
    NAME TestForEachDelta
    STDOUT ${TEST_CASE_ROOT}/stdout
    library-cpp-libgit2_wrapper-ut TRepositoryTests::TestForEachDelta
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/stdout
)

DATA(
    sbr://2880875763
)

DEPENDS(
    library/cpp/libgit2_wrapper/ut
)

END()
