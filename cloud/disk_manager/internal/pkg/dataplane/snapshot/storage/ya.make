OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    common.go
    factory.go
    storage.go
    storage_legacy.go
    storage_legacy_impl.go
    storage_ydb.go
    storage_ydb_impl.go
)

GO_TEST_SRCS(
    storage_ydb_test.go
)

END()

RECURSE(
    chunks
    compressor
    metrics
    protos
    schema
)

RECURSE_FOR_TESTS(
    tests
)
