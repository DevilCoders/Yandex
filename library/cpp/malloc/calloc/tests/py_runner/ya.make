PY2TEST()

OWNER(iddqd)

DEPENDS(
    library/cpp/malloc/calloc/tests/do_with_lf
    library/cpp/malloc/calloc/tests/do_with_enabled
    library/cpp/malloc/calloc/tests/do_with_disabled
)

TEST_SRCS(test_enabled.py)

END()
