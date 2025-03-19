GO_LIBRARY()

SRCS(
    queries.go
)

GO_TEST_SRCS(
    main_test.go
    queries_test.go
)

END()

RECURSE(gotest)
