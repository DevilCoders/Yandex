OWNER(alzobnin)

PY2TEST()

TEST_SRCS(test_untranslit.py)

DATA(
    arcadia/tools/untranslit_test
    arcadia_tests_data/lemmer/translit
)

DEPENDS(tools/untranslit_test)



END()
