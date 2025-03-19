GO_LIBRARY()

SRCS(
    cumulative.go
    invalid.go
    oversized.go
    queries.go
    schemes.go
)

GO_TEST_SRCS(
    cumulative_test.go
    invalid_test.go
    main_test.go
    oversized_test.go
)

END()

RECURSE(gotest)
