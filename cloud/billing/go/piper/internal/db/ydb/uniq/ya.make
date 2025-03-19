GO_LIBRARY()

SRCS(
    hashed.go
    queries.go
    schemes.go
)

GO_TEST_SRCS(
    hashed_test.go
    main_test.go
    schemes_test.go
)

END()

RECURSE(gotest)
