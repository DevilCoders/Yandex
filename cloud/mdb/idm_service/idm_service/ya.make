OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/Flask
    contrib/python/flask-appconfig
    contrib/python/Flask-RESTful
    contrib/python/psycopg2
    contrib/python/PyNaCl
    contrib/python/webargs/py2
    library/python/blackbox
    library/python/tvm2
    library/python/tvmauth
    library/python/vault_client
)

PY_SRCS(
    NAMESPACE idm_service
    __init__.py
    dbutils.py
    default_config.py
    exceptions.py
    metadb.py
    mysql_metadb.py
    mysql_pillar.py
    mysql_schemas.py
    pg_metadb.py
    pg_pillar.py
    pg_schemas.py
    pillar.py
    schemas.py
    tasks_version.py
    tvm_constants.py
    utils.py
    vault.py
    views.py
)

END()
