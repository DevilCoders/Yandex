PROTO_LIBRARY()

OWNER(g:mdb)

PY_NAMESPACE(cloud.mdb.mlock.api)

INCLUDE_TAGS(GO_PROTO)

GRPC()

SRCS(
    lock.proto
    lock_service.proto
)

IF (GO_PROTO)
    PEERDIR(
        vendor/google.golang.org/genproto/googleapis/rpc/code
        vendor/google.golang.org/genproto/googleapis/rpc/errdetails
        vendor/google.golang.org/genproto/googleapis/rpc/status
    )
ELSE()
    PEERDIR(contrib/libs/googleapis-common-protos)
ENDIF()

END()

RECURSE(mocks)
