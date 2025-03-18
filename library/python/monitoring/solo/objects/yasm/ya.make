OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    alert.py
    base.py
    chart.py
    panel.py
)

PEERDIR(
    library/python/monitoring/solo/objects/common
    contrib/python/jsonobject
    contrib/python/six
    library/python/monitoring/solo/util
)

END()
