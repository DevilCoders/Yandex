PROTO_LIBRARY()

OWNER(g:cloud-iam)


PROTO_NAMESPACE(
    GLOBAL cloud/iam/accessservice/client/iam-access-service-client-cpp/1.0.0/submodules/iam-access-service-client-proto/private-api
)

GRPC()

SRCS(
    access_service.proto
    resource.proto
)

EXCLUDE_TAGS(
    GO_PROTO
    PY_PROTO
)

USE_COMMON_GOOGLE_APIS(
    api/annotations
    rpc/code
    rpc/errdetails
    rpc/status
    type/timeofday
    type/dayofweek
)

END()

