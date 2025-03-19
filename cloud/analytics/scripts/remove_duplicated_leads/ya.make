PY3_PROGRAM()

OWNER(g:cloud_analytics)

PEERDIR(
    cloud/analytics/scripts/common

    contrib/python/click
    contrib/python/tqdm
    yt/python/client_with_rpc
)

PY_SRCS(MAIN __init__.py)

END()
