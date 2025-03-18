GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(
    cache.go
    inspect.go
)

GO_TEST_SRCS(inspect_test.go)

END()

RECURSE(gotest)
