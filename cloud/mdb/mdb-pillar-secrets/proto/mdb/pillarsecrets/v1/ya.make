OWNER(g:datacloud)

PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/mdb/mdb-pillar-secrets)
ONLY_TAGS(GO_PROTO)
GRPC()
SRCS(pillar_secret_service.proto)

USE_COMMON_GOOGLE_APIS(
    rpc/status
)

END()
