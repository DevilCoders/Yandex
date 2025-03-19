GO_LIBRARY()

OWNER(g:mdb)

TAG(ya:manual)

SRCS(
    matchers.go
)

GO_TEST_SRCS(
    matchers_test.go
    mysync_test.go
)

END()

RECURSE(gotest)
