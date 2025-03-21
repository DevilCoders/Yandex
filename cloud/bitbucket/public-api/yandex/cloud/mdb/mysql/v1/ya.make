# This file is generated by
# https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/tools/cmd/syncapi
# in Teamcity Task
# https://teamcity.yandex-team.ru/buildConfiguration/Cloud_Api_ApiSync

OWNER(g:cloud-api g:mdb)
PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/bitbucket/public-api)
PY_NAMESPACE(yandex.cloud.mdb.mysql.v1)

GRPC()
SRCS(
    backup.proto
    backup_service.proto
    cluster.proto
    cluster_service.proto
    database.proto
    database_service.proto
    maintenance.proto
    resource_preset.proto
    resource_preset_service.proto
    user.proto
    user_service.proto
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
        cloud/bitbucket/common-api/yandex/cloud/api
        cloud/bitbucket/common-api/yandex/cloud/api/tools
        cloud/bitbucket/public-api/yandex/cloud
        cloud/bitbucket/public-api/yandex/cloud/mdb/mysql/v1/config
        cloud/bitbucket/public-api/yandex/cloud/operation
    )
ELSE()
    PEERDIR(
        cloud/bitbucket/common-api/yandex/cloud/api
        cloud/bitbucket/common-api/yandex/cloud/api/tools
        cloud/bitbucket/public-api/yandex/cloud
        cloud/bitbucket/public-api/yandex/cloud/mdb/mysql/v1/config
        cloud/bitbucket/public-api/yandex/cloud/operation
    )
ENDIF()
END()

