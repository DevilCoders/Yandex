GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    base.go
    elasticsearch.go
    greenplum.go
    mongodb.go
    mysql.go
    postgres.go
    redis.go
    zookeeper.go
)

GO_XTEST_SRCS(
    elasticsearch_test.go
    mongodb_test.go
    redis_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
