GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    models.go
    pg.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(gotest)
