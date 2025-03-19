GO_LIBRARY()

OWNER(
    tolich
    g:vh
    g:music-sre
)

SRCS(
    auth.go
    flags.go
    handler.go
    serve.go
)

GO_TEST_SRCS(auth_test.go)

END()

RECURSE(gotest)
