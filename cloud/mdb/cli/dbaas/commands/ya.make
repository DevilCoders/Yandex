OWNER(g:mdb)

PY3_LIBRARY()

FORK_TESTS()

ALL_PY_SRCS()

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v1/console
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/servicecontrol/v1
    cloud/mdb/cli/common

    contrib/python/protobuf
)

END()
