GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    batcher.go
    exporter.go
)

GO_TEST_SRCS(exporter_test.go)

END()

RECURSE(
    backup
    cloudstorage
    gotest
    model
)
