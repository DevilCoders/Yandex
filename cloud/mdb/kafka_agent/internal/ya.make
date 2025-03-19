PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1/inner
    cloud/mdb/datacloud/private_api/datacloud/kafka/inner/v1
    cloud/mdb/internal/python/compute/iam/jwt
    cloud/mdb/internal/python/grpcutil
    cloud/mdb/internal/python/logs
    contrib/python/confluent-kafka
    contrib/python/py4j
    contrib/python/tenacity
)

PY_SRCS(
    __init__.py
    topic_sync.py
)

END()
