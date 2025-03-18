GO_PROGRAM()

OWNER(
    gzuykov
    buglloc
    g:go-library
)

SRCS(main.go)

GO_TEST_SRCS(main_test.go)

END()

RECURSE(gotest)
