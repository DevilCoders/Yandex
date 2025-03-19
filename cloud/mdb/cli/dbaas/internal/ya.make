OWNER(g:mdb)

PY3_LIBRARY()

FORK_TESTS()

ALL_PY_SRCS(RECURSIVE)

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/vpc/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1
    cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1
    cloud/mdb/cms/api/grpcapi/v1
    cloud/mdb/cli/common

    contrib/libs/grpc/src/python/grpcio_status
    contrib/python/boto3
    contrib/python/Jinja2
    contrib/python/juggler_sdk
    contrib/python/PyNaCl
    contrib/python/psycopg2
    contrib/python/PyJWT
    contrib/python/requests
    contrib/python/sqlparse
    contrib/python/tenacity
    contrib/python/sshtunnel
)

END()
