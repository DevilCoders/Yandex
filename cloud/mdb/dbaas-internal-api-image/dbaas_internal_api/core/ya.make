OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.core
    __init__.py
    auth.py
    base_pillar.py
    crypto.py
    exceptions.py
    flask.py
    id_generators.py
    logs.py
    middleware.py
    raven.py
    stat.py
    tracing.py
    types.py
)

END()
