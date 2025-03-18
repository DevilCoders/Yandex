OWNER(alzobnin)

PY2TEST()

TEST_SRCS(tools_queryrectest.py)

DATA(
    arcadia_tests_data/recognize/queryrec
    arcadia_tests_data/wizard/language
    arcadia/tools/queryrectest/tests/data
    sbr://107284261
)

DEPENDS(tools/queryrectest)

FORK_SUBTESTS()

END()
