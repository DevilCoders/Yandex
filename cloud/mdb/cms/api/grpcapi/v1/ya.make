PROTO_LIBRARY()

OWNER(g:mdb)

GRPC()

SRCS(
    duty_service.proto
    instance_operation.proto
    instance_operation_service.proto
    instance_service.proto
)

USE_COMMON_GOOGLE_APIS(rpc/status)

END()
