OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.apis.schemas
    __init__.py
    alerts.py
    backups.py
    cluster.py
    common.py
    console.py
    fields.py
    maintenance.py
    objects.py
    operations.py
    quota.py
    quota_usage.py
    search.py
    support.py
)

END()
