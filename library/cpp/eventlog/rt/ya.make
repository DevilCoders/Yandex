EXECTEST()

OWNER(
    vmordovin
    avitella
)

RUN(
    NAME
    eventlog_test
    tool
    STDOUT
    ${TEST_OUT_ROOT}/eventlog_test.out
    CANONIZE
    ${TEST_OUT_ROOT}/eventlog_test.out
)

DEPENDS(library/cpp/eventlog/rt/tool)

END()
