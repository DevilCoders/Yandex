PY3_PROGRAM(overcommit_factor)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/ycp

    contrib/python/numpy
    contrib/python/pandas
    contrib/python/requests
)

PY_SRCS(
    __main__.py
)

END()
