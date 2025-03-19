PY2_LIBRARY()

OWNER(g:cloud-iam)

PY_SRCS(
    backup.py
    ydb_reader.py
)

PEERDIR(
    contrib/python/futures
    contrib/python/protobuf
    ydb/public/api/protos
    yt/python/client
    #contrib/deprecated/python/subprocess32
    #library/python/resource
    #ydo/database/util
    #yql/library/python
)

END()
