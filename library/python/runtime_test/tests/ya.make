PY2TEST()

OWNER(pg)

DEPENDS(
    library/python/runtime_test/py2_prog
    library/python/runtime_test/py3_prog
)

TEST_SRCS(
    test_main_py_support.py
)

END()
