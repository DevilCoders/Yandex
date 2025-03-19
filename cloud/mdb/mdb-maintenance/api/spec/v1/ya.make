PROTO_LIBRARY()

OWNER(g:mdb)

ONLY_TAGS(GO_PROTO)

PY_NAMESPACE(cloud.mdb.maintainance.api.v1)

GRPC()

SRCS(maintenance_service.proto)

USE_COMMON_GOOGLE_APIS(rpc/status)

END()
