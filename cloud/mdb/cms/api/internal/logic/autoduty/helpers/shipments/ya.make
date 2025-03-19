GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    await_shipment.go
    exec_shell.go
    format.go
    wrapper_result.go
)

GO_XTEST_SRCS(exec_shell_test.go)

END()

RECURSE_FOR_TESTS(gotest)
