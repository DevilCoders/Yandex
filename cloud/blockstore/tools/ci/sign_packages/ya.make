PY3_PROGRAM(yc-nbs-ci-sign-packages)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/common

    contrib/python/requests
)

END()
