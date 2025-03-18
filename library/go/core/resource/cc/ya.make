GO_PROGRAM()

OWNER(
    prime
    g:go-library
)

SRCS(main.go)

GO_TEST_SRCS(generate_test.go)

END()

RECURSE(gotest)
