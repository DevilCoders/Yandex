OWNER(
    g:ugc
    stakanviski
)

EXECTEST()

RUN(
    schemac --dump test.schema
    STDOUT ${TEST_OUT_ROOT}/test.dump
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/test.dump
)

DATA(
    arcadia/kernel/ugc/schema/compiler/bin/exectest
)

DEPENDS(
    kernel/ugc/schema/compiler/bin
)

TEST_CWD(kernel/ugc/schema/compiler/bin/exectest)

END()
