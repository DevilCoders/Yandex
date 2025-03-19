PY3_LIBRARY()

OWNER(
    soin08
)


PEERDIR(
    cloud/dwh/lms/controllers
    cloud/dwh/lms/hooks
    cloud/dwh/lms/models
    cloud/dwh/lms/repositories
    cloud/dwh/lms/services
    cloud/dwh/lms/utils
    contrib/python/PyYAML
    contrib/python/psycopg2
    library/python/vault_client
)

PY_SRCS(
    __init__.py
    exceptions.py
    config.py
)

END()

RECURSE(
    controllers
    hooks
    models
    repositories
    services
    utils
)
