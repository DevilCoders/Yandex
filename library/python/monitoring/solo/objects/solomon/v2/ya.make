OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
    alert.py
    base.py
    channel.py
    cluster.py
    dashboard.py
    graph.py
    menu.py
    project.py
    service.py
    shard.py
)

PEERDIR(
    contrib/python/jsonobject
    contrib/python/six
    library/python/monitoring/solo/util
    library/python/monitoring/solo/objects/common
)

END()

RECURSE_FOR_TESTS(
    ut
)
