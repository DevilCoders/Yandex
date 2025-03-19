# Build as application to force dependency build
PY3_PROGRAM(python-protobuf)

STYLE_PYTHON()

OWNER(g:cloud)

PY_SRCS()

PEERDIR(
    cloud/mdb/mlock/api
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1/console
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/opensearch/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/redis/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1
)

END()
