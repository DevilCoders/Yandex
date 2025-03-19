GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    context.go
    list.go
    setup.go
    time.go
    yatest.go
)

GO_TEST_SRCS(time_test.go)

GO_XTEST_SRCS(
    inputs_test.go
    list_test.go
    setup_test.go
)

END()

RECURSE(gotest)
