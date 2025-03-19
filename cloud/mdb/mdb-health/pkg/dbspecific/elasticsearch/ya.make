GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    elasticsearch.go
    tests.go
)

GO_TEST_SRCS(elasticsearch_test.go)

END()

RECURSE(gotest)
