PY3_LIBRARY()

OWNER(g:cloud_analytics)

PEERDIR(
    yt/python/client

    crypta/lib/python/identifiers
)

PY_SRCS(
    leads_dyn_table.py
)

END()
