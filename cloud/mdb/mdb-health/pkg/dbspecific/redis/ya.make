GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    redis.go
    tests.go
)

GO_TEST_SRCS(redis_test.go)

END()

RECURSE(gotest)
