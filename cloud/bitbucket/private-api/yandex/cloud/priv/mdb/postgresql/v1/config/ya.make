# This file is generated by
# https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/tools/cmd/syncapi
# in Teamcity Task
# https://teamcity.yandex-team.ru/buildConfiguration/Cloud_Api_ApiSync

OWNER(g:cloud-api g:mdb)
PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/bitbucket/private-api)
PY_NAMESPACE(yandex.cloud.priv.mdb.postgresql.v1.config)

GRPC()
SRCS(
    host10.proto
    host10_1c.proto
    host11.proto
    host11_1c.proto
    host12.proto
    host12_1c.proto
    host13.proto
    host13_1c.proto
    host14.proto
    host14_1c.proto
    host9_6.proto
    postgresql10.proto
    postgresql10_1c.proto
    postgresql11.proto
    postgresql11_1c.proto
    postgresql12.proto
    postgresql12_1c.proto
    postgresql13.proto
    postgresql13_1c.proto
    postgresql14.proto
    postgresql14_1c.proto
    postgresql9_6.proto
)

USE_COMMON_GOOGLE_APIS(
    api/annotations
    rpc/code
    rpc/errdetails
    rpc/status
    type/timeofday
    type/dayofweek
)

IF (GO_PROTO)
    PEERDIR(
        cloud/bitbucket/private-api/yandex/cloud/priv
    )
ELSE()
    PEERDIR(
        cloud/bitbucket/private-api/yandex/cloud/priv
    )
ENDIF()
END()

