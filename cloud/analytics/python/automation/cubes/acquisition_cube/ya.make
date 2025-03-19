OWNER(g:cloud_analytics)

PY2_PROGRAM()

PEERDIR(nirvana/valhalla/src)
PEERDIR(cloud/analytics/nirvana/vh/config)

PEERDIR(contrib/python/click)
PEERDIR(contrib/python/requests)
PEERDIR(yt/python/client)
PEERDIR(contrib/python/pandas)
PEERDIR(contrib/python/PyMySQL)
PEERDIR(statbox/nile)
PEERDIR(yt/yt/python/yt_yson_bindings)

PY_SRCS(MAIN get_calls_from_mysql_to_yt.py)

END()
