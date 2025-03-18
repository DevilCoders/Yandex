GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(
    recurse.go
    remove_lines.go
)

GO_TEST_SRCS(remove_lines_test.go)

END()

RECURSE(gotest)
