GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backups.go
    clusters.go
    greenplum.go
    search.go
)

GO_TEST_SRCS(clusters_test.go)

END()

RECURSE(
    gotest
    internal
)
