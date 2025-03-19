PY3_PROGRAM()

OWNER(g:cloud-ai)

PY_SRCS(
    MAIN build_disk_snapshot.py
)

PEERDIR(
    # contrib/python/requests
)

END()
