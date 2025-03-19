PROTO_LIBRARY()

OWNER(
    tserakhau
    g:data-transfer
)

GRPC()

PEERDIR(
    cloud/dataplatform/api/schemaregistry/options
    transfer_manager/go/proto/api/options
)

USE_COMMON_GOOGLE_APIS(
    api/annotations
    rpc/code
    rpc/status
)

SET_APPEND(PROTO_FILES
    namespace.proto
    namespace_service.proto
    schema.proto
    schema_service.proto
    search_service.proto
    version_service.proto
)

IF(GO_PROTO)
    PEERDIR(
        ${GOSTD}/regexp
        library/go/core/xerrors
    )
ENDIF()

SRCS(${PROTO_FILES})

GO_GRPC_GATEWAY_SWAGGER_SRCS(
    namespace_service.proto
    schema_service.proto
    search_service.proto
    version_service.proto
)

IF (GO_PROTO)
    RESOURCE(
        cloud/dataplatform/api/schemaregistry/namespace_service.swagger.json namespace_service.swagger.json
        cloud/dataplatform/api/schemaregistry/schema_service.swagger.json schema_service.swagger.json
        cloud/dataplatform/api/schemaregistry/search_service.swagger.json search_service.swagger.json
        cloud/dataplatform/api/schemaregistry/version_service.swagger.json version_service.swagger.json
    )
ENDIF()

GO_PROTO_PLUGIN(genvalidate .vld.go transfer_manager/go/cmd/genvalidate)
GO_PROTO_PLUGIN(genoneof .oneof.go transfer_manager/go/cmd/genoneof)

END()

RECURSE(
    options
)
