PY3TEST()

OWNER(g:cloud-nbs)

SIZE(SMALL)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/blockstore/tools/ci/migration_test
)

END()
