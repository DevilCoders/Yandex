PY3_PROGRAM(build-test-container)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/clients/sandbox
)

END()
