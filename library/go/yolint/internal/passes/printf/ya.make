GO_LIBRARY()

OWNER(
    buglloc
    g:go-library
)

SRCS(printf.go)

GO_XTEST_SRCS(printf_test.go)

END()

RECURSE(gotest)
