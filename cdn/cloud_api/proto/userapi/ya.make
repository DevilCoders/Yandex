PROTO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

ONLY_TAGS(GO_PROTO)

GRPC()

USE_COMMON_GOOGLE_APIS(api/annotations)

SRCS(
    origin_service.proto
    origins_group_service.proto
    resource_params.proto
    resource_rule_service.proto
    resource_service.proto
)

GO_GRPC_GATEWAY_SWAGGER_SRCS(
    origin_service.proto
    origins_group_service.proto
    resource_rule_service.proto
    resource_service.proto
)

PEERDIR(cdn/cloud_api/proto/model)

IF (GO_PROTO)
    RESOURCE_FILES(
        PREFIX
        specs/
        origin_service.swagger.json
        origins_group_service.swagger.json
        resource_rule_service.swagger.json
        resource_service.swagger.json
    )
ENDIF()

END()
