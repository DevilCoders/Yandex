GO_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    # imports must be specified explicitly
    ${GOSTD}/context
    cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1
    cloud/mdb/datacloud/private_api/datacloud/v1
    vendor/google.golang.org/grpc
)

INCLUDE(mockgen.inc)

GO_MOCKGEN_MOCKS()

END()

RECURSE(gen)
