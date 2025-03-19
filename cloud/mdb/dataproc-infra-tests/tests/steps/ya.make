OWNER(g:mdb-dataproc)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/behave
    contrib/python/deepdiff
    contrib/python/PyHamcrest
    contrib/python/humanfriendly
    contrib/python/PyYAML
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1
)

PY_SRCS(
    NAMESPACE tests.steps
    __init__.py
    common.py
    go_internal_api.py
    greenplum_cluster.py
    hadoop_cluster.py
    internal_api.py
    kafka_cluster.py
    metadb.py
    metastore_cluster.py
    sqlserver_cluster.py
    yc_cli.py
)

END()
