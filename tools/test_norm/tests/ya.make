OWNER(akhropov)

PY2TEST()

TEST_SRCS(test_norm.py)

DATA(
    arcadia_tests_data/wizard/synnorm
    arcadia_tests_data/wizard/language
)

DEPENDS(tools/test_norm)



END()
