GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(registryutil.go)

GO_TEST_SRCS(registryutil_test.go)

END()

RECURSE(gotest)
