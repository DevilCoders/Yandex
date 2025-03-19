GO_LIBRARY()

SRCS(
    context.go
    queries.go
)

GO_TEST_SRCS(
    context_test.go
    main_test.go
)

END()

RECURSE(gotest)
