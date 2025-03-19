GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    actions.go
    funcs.go
    generate.go
    models.go
    operator.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(
    gotest
    mocks
    provider
)
