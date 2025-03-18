OWNER(g:antirobot)

PROTO_LIBRARY()

INCLUDE_TAGS(GO_PROTO)

GRPC()

PEERDIR(
    cloud/bitbucket/common-api/yandex/cloud/api
    cloud/bitbucket/private-api/yandex/cloud/priv
    cloud/bitbucket/private-api/yandex/cloud/priv/operation
    cloud/bitbucket/private-api/yandex/cloud/priv/quota
    cloud/bitbucket/private-api/yandex/cloud/priv/smartcaptcha/v1
)

USE_COMMON_GOOGLE_APIS(
    api/annotations
    rpc/code
    rpc/status
)

IF(GO_PROTO)
    PEERDIR(library/go/core/xerrors)
ENDIF()

SRCS(
    log.proto
    ydb_schema.proto
)

IF(GO_PROTO)
    PEERDIR(library/go/core/xerrors)
ENDIF()


END()
