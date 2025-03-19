OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.apis.console
    __init__.py
    billing.py
    clusters.py
    restore_hints.py
)

END()
