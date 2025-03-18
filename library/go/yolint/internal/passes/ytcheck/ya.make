GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(ytcheck.go)

GO_TEST_SRCS(ytcheck_test.go)

END()

RECURSE(gotest)
