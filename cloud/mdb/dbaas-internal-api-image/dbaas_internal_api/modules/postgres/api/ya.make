OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.postgres.api
    __init__.py
    convertors.py
)

END()
