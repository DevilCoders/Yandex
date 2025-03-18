GO_LIBRARY()

OWNER(
    buglloc
    g:go-library
)

SRCS(
    root.go
    ya.go
)

GO_XTEST_SRCS(
    root_example_test.go
    root_test.go
    ya_example_test.go
    ya_test.go
)

END()

RECURSE(gotest)
