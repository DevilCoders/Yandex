GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(
    migration.go
    named.go
    nogen.go
    nolint.go
)

GO_TEST_SRCS(
    migration_test.go
    named_test.go
    nogen_test.go
)

END()

RECURSE(gotest)
