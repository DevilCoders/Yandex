GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    auth.go
    dashboards.go
    modify.go
    opensearch.go
)

GO_TEST_SRCS(
    auth_test.go
    clusters_test.go
    opensearch_test.go
)

END()

RECURSE(gotest)
