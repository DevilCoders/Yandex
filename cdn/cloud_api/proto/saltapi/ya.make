PROTO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

ONLY_TAGS(GO_PROTO)

GRPC()

USE_COMMON_GOOGLE_APIS(api/annotations)

SRCS(
    saltapi.proto
    saltapi_params.proto
)

GO_GRPC_GATEWAY_SWAGGER_SRCS(saltapi.proto)

PEERDIR(cdn/cloud_api/proto/model)

IF (GO_PROTO)
    RESOURCE_FILES(
        PREFIX
        specs/
        saltapi.swagger.json
    )
ENDIF()

END()
