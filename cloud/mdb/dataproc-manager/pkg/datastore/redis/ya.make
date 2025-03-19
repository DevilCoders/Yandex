GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(redis.go)

GO_TEST_SRCS(redis_test.go)

END()

RECURSE(gotest)
