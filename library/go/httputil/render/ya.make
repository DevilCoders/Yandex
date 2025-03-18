GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(renderer.go)

GO_XTEST_SRCS(renderer_test.go)

END()

RECURSE(
    gotest
    testproto
)
