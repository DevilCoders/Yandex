GO_LIBRARY()

OWNER(
    g:go-library
    g:kikimr
)

SRCS(
    tvm.go
)

GO_TEST_SRCS(
    tvm_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
