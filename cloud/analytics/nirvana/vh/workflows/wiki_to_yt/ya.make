OWNER(g:cloud_analytics)

PY2_PROGRAM()

PEERDIR(
    nirvana/valhalla/src
    library/python/reactor/client
    cloud/analytics/nirvana/vh/config
    cloud/analytics/nirvana/vh/utils

    contrib/python/click
    contrib/python/requests
    yt/python/client
    contrib/python/retry
    yt/yt/python/yt_yson_bindings
)

PY_SRCS(MAIN wiki_to_yt.py)

END()
