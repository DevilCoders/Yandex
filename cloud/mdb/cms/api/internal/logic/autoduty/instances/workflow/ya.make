GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    base.go
    log.go
    move_instance.go
    run.go
    whip_primary.go
)

GO_TEST_SRCS(base_test.go)

GO_XTEST_SRCS(run_test.go)

END()

RECURSE_FOR_TESTS(gotest)
