PY2TEST()

OWNER(g:tools-python)


PEERDIR(
    library/python/tvm2/tests
)

DATA(
    arcadia/library/python/tvm2/vcr_cassettes
)

TEST_CWD(library/python/tvm2/vcr_cassettes)

END()
