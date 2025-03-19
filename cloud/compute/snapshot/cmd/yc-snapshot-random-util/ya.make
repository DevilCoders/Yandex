GO_PROGRAM()

OWNER(g:cloud-nbs)

SRCS(main.go)

GO_TEST_SRCS(main_test.go)

END()

RECURSE(
    gotest
    internal
)
