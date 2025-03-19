PY3TEST()

OWNER(g:cloud-nbs)

SIZE(SMALL)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/filestore/tests/multiclient-rw-test/runner
)

END()
