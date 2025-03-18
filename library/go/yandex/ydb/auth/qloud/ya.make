GO_LIBRARY()

OWNER(
    g:go-library
    g:kikimr
)

SRCS(
    qloud.go
)

GO_TEST_SRCS(
    qloud_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
