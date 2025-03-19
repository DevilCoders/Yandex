PY3_PROGRAM(yc-nbs-ci-run-ya-make)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/clients/sandbox
    cloud/blockstore/pylibs/common

    contrib/python/requests

    sandbox/common/types
)

END()
