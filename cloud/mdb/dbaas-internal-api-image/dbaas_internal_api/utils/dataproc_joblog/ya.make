OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.utils.dataproc_joblog
    __init__.py
    api.py
    client.py
)

END()
