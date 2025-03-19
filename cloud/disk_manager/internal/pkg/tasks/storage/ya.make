OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    common.go
    metrics.go
    compound_storage.go
    storage.go
    storage_ydb.go
    storage_ydb_impl.go
)

GO_TEST_SRCS(
    storage_ydb_test.go
)

END()

RECURSE(
    protos
)

RECURSE_FOR_TESTS(
    mocks
    tests
)
