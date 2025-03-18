OWNER(
    g:solo
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    menu.py
)

PEERDIR(
    library/python/monitoring/solo/example/example_project/registry/project
    library/python/monitoring/solo/example/example_project/registry/dashboard
    library/python/monitoring/solo/objects/solomon/v2
)

END()
