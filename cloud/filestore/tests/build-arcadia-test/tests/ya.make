PY3TEST()

OWNER(g:cloud-nbs)

SIZE(SMALL)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/filestore/tests/build-arcadia-test
)

END()
