GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.go
    opentracingutils.go
    ydbtrace.go
)

GO_TEST_SRCS(opentracingutils_test.go)

END()

RECURSE(
    gotest
    grpcinterceptors
    httpmiddleware
)
