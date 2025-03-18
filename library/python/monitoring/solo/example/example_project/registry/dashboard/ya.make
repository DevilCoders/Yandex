OWNER(
    g:solo
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    dashboards_v3.py
    dashboards.py
    juggler_dashboard.py
)

PEERDIR(
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/graph
    library/python/monitoring/solo/objects/solomon/v2
    library/python/monitoring/solo/util
    library/python/monitoring/solo/objects/solomon/v3
    library/python/monitoring/solo/objects/juggler
    library/python/monitoring/solo/helpers
)

END()
