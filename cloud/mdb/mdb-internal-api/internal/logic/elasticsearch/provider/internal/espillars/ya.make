GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    auth.go
    elaticsearch.go
    kibana.go
    modify.go
)

GO_TEST_SRCS(
    auth_test.go
    clusters_test.go
    elasticsearch_test.go
)

END()

RECURSE(gotest)
