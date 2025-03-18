OWNER(
    g:solo
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    sensors.py
)

PEERDIR(
    library/python/monitoring/solo/objects/solomon/sensor
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/cluster
    library/python/monitoring/solo/example/example_project/registry/service
)

END()
