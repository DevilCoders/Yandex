PY2TEST()

OWNER(g:tools-python)

PEERDIR(
    library/python/dir-sync
)

TEST_SRCS(
    __init__.py
    org_ctx.py
    organization_sync_statistics.py
)

END()
