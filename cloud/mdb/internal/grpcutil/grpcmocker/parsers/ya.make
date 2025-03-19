GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    matchers.go
    parser.go
    responders.go
)

GO_TEST_SRCS(
    matchers_test.go
    responders_test.go
)

END()

RECURSE(gotest)
