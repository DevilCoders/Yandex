OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    contrib/python/retrying
    contrib/python/requests
    contrib/python/psycopg2
    contrib/python/jsonschema
    contrib/python/clickhouse-driver
    contrib/python/pymongo
    contrib/python/PyMySQL
    contrib/python/redis
    contrib/python/confluent-kafka
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1
    cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1
    cloud/mdb/datacloud/private_api/datacloud/kafka/v1
)

PY_SRCS(
    NAMESPACE dbaas_e2e.scenarios
    __init__.py
    clickhouse_cluster.py
    clickhouse_doublecloud_cluster.py
    clickhouse_cloud_storage_cluster.py
    hadoop_cluster.py
    mongodb_cluster.py
    mysql_cluster.py
    postgresql_cluster.py
    redis_cluster.py
    elasticsearch_cluster.py
    sqlserver_cluster.py
    kafka_cluster.py
    kafka_doublecloud_cluster.py
    greenplum_cluster.py
)

END()
