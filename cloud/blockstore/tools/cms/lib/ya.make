PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    cms.py
    conductor.py
    config.py
    proto.py
    pssh.py
    tools.py
)

PEERDIR(
    cloud/blockstore/config
    contrib/python/requests
    ydb/core/protos
    ydb/public/api/protos
)

END()
