GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    kafka.go
    tests.go
)

GO_TEST_SRCS(kafka_test.go)

END()

RECURSE(gotest)
