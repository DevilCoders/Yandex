PROTO_LIBRARY()

OWNER(
    tserakhau
    g:data-transfer
)

GRPC()

PEERDIR(
    cloud/bitbucket/common-api/yandex/cloud/api
    cloud/bitbucket/private-api/yandex/cloud/priv
    cloud/bitbucket/private-api/yandex/cloud/priv/operation
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
    clickhouse.proto
    common.proto
    connection.proto
    connection_operation.proto
    connection_service.proto
    kafka.proto
    mongodb.proto
    mysql.proto
    postgresql.proto
    redis.proto
)

IF(GO_PROTO)
    PEERDIR(library/go/core/xerrors)
ENDIF()

GO_GRPC_GATEWAY_SWAGGER_SRCS(
    connection_service.proto
)

IF(GO_PROTO)
    RESOURCE(
        cloud/dataplatform/api/connman/connection_service.swagger.json connection_service.swagger.json
    )
ENDIF()

GO_PROTO_PLUGIN(
    genvalidate .vld.go transfer_manager/go/cmd/genvalidate
)

END()
