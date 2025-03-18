GO_LIBRARY()

OWNER(
    g:library-go
    gzuykov
)

SRCS(structtagcase.go)

GO_TEST_SRCS(structtagcase_test.go)

END()

RECURSE(gotest)
