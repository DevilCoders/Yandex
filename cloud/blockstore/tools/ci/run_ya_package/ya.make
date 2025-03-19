PY3_PROGRAM(yc-nbs-ci-run-ya-package)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/clients/sandbox
    cloud/blockstore/pylibs/common
)

END()
