PY3TEST()

OWNER(g:tools-python)


PEERDIR(
    library/python/tvm2/tests
    contrib/python/pytest-asyncio
)

DATA(
    arcadia/library/python/tvm2/vcr_cassettes
)

TEST_CWD(library/python/tvm2/vcr_cassettes)

END()
