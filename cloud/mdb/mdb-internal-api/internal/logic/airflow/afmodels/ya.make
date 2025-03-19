GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    config.go
    operation.go
    tasks.go
    version.go
)

GO_TEST_SRCS(version_test.go)

END()

RECURSE(gotest)
