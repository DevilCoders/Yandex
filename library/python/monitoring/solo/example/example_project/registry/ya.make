OWNER(
    g:solo
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
)

PEERDIR(
    library/python/monitoring/solo/example/example_project/registry/alert
    library/python/monitoring/solo/example/example_project/registry/graph
    library/python/monitoring/solo/example/example_project/registry/channel
    library/python/monitoring/solo/example/example_project/registry/cluster
    library/python/monitoring/solo/example/example_project/registry/dashboard
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/sensor
    library/python/monitoring/solo/example/example_project/registry/service
    library/python/monitoring/solo/example/example_project/registry/shard
    library/python/monitoring/solo/example/example_project/registry/menu

)

END()

RECURSE(
    alert
    channel
    cluster
    dashboard
    graph
    menu
    project
    sensor
    service
    shard
)
