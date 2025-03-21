# This file is generated by
# https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/tools/cmd/syncapi
# in Teamcity Task
# https://teamcity.yandex-team.ru/buildConfiguration/Cloud_Api_ApiSync

OWNER(g:cloud-api)
PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/bitbucket/public-api)
PY_NAMESPACE(yandex.cloud.containerregistry.v1)

GRPC()
SRCS(
    blob.proto
    image.proto
    image_service.proto
    ip_permission.proto
    lifecycle_policy.proto
    lifecycle_policy_service.proto
    registry.proto
    registry_service.proto
    repository.proto
    repository_service.proto
    scanner.proto
    scanner_service.proto
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
        cloud/bitbucket/public-api/yandex/cloud/access
        cloud/bitbucket/public-api/yandex/cloud/operation
    )
ELSE()
    PEERDIR(
        cloud/bitbucket/common-api/yandex/cloud/api
        cloud/bitbucket/common-api/yandex/cloud/api/tools
        cloud/bitbucket/public-api/yandex/cloud
        cloud/bitbucket/public-api/yandex/cloud/access
        cloud/bitbucket/public-api/yandex/cloud/operation
    )
ENDIF()
END()

