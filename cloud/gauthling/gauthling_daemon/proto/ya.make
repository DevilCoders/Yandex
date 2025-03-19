OWNER(g:cloud)

PROTO_LIBRARY()

GRPC()

PEERDIR(
    cloud/bitbucket/private-api/yandex/cloud/priv
)

SRCS(
    gauthling.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
