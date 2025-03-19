PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

FORK_TESTS()

PEERDIR(
    contrib/python/psycopg2
    contrib/python/pyaml
    contrib/python/humanfriendly
    contrib/python/requests
    cloud/mdb/internal/python/ipython_repl
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/logs
    cloud/mdb/internal/python/compute/iam/jwt
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1
)

PY_SRCS(
    __init__.py
    auth.py
    config.py
    cleaner.py
    internal_api.py
    logic.py
    metadb.py
    models.py
)

END()
