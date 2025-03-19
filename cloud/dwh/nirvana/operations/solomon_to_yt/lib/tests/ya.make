OWNER(g:cloud-dwh)

PY3TEST()

SIZE(SMALL)

PEERDIR(
    contrib/python/pytest-asyncio

    cloud/dwh/nirvana/operations/solomon_to_yt/lib
)

TEST_SRCS(
    test_operation.py
    test_types.py
)

END()
