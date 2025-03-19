GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    auth.go
    backups.go
    clusters.go
    console.go
    extension.go
    modify.go
    opensearch.go
    search.go
)

GO_TEST_SRCS(
    clusters_test.go
    opensearch_test.go
)

END()

RECURSE(
    gotest
    internal
)
