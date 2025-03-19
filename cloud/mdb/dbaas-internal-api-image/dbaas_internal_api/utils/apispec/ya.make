OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.utils.apispec
    __init__.py
    apispec_v0_3_9.py
    apispec_v1_3_3.py
)

NO_CHECK_IMPORTS(
    dbaas_internal_api.utils.apispec.apispec_v1_3_3
)

END()
