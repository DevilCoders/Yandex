PY23_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    report.py
    trace.py
)

PEERDIR(
    cloud/blockstore/tools/analytics/common

    contrib/python/lxml
)

END()
