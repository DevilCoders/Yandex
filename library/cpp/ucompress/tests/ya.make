OWNER(g:ymake)

PY3TEST()

DEPENDS(
    tools/uc
    library/cpp/ucompress/tests/tool
)

TEST_SRCS(
    test.py
)

SIZE(MEDIUM)

END()

RECURSE(
    tool
)
