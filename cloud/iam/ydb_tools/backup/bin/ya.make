PY2_PROGRAM(iam_backup)

OWNER(g:cloud-iam)

PEERDIR(
    cloud/iam/ydb_tools/backup/lib
)

PY_SRCS(
    __main__.py
)

END()
