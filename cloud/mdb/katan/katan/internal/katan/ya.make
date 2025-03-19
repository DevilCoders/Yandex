GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    katan.go
    runner.go
    sentry.go
)

GO_XTEST_SRCS(
    runner_test.go
    sentry_test.go
)

END()

RECURSE(gotest)
