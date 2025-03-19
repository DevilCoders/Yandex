PY3_PROGRAM()

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/tools/cms/lib

    contrib/python/requests
)

PY_SRCS(
    __main__.py
)

END()

RECURSE_FOR_TESTS(tests)
