GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    compute.go
    config.go
    last.go
)

GO_TEST_SRCS(compute_test.go)

END()

RECURSE(gotest)
