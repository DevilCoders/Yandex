PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

SIZE(MEDIUM)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1/inner
    cloud/mdb/datacloud/private_api/datacloud/kafka/inner/v1
    cloud/mdb/kafka_agent/internal
)

TEST_SRCS(test_topic_sync.py)

END()
