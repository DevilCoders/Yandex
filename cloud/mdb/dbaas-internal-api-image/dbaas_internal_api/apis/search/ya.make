OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.apis.search
    __init__.py
    reindex.py
)

END()
