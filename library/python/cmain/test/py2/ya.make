EXECTEST()

OWNER(borman)

DEPENDS(library/python/cmain/example/py2)

RUN(
    example
    test
    1
    2
    3
    foo
    bar
    STDOUT
    ${TEST_OUT_ROOT}/out
    CANONIZE_LOCALLY
    ${TEST_OUT_ROOT}/out
)

END()
