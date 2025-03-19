GO_LIBRARY()

SRCS(
    invalid.go
    queries.go
)

GO_TEST_SRCS(
    invalid_test.go
    main_test.go
)

END()

RECURSE(gotest)
