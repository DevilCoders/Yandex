GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    base.go
    config.go
    explanations.go
    instructions.go
    request_batch.go
    request_single.go
    run.go
    take_requests.go
)

GO_XTEST_SRCS(request_single_test.go)

END()

RECURSE(
    gotest
    integrationtests
)
