GO_LIBRARY()

OWNER(tserakhau)

SRCS(
    postgres.go
    queries.go
    repository.go
)

GO_XTEST_SRCS(repository_test.go)

GO_EMBED_PATTERN(migrations)

END()

RECURSE(gotest)
