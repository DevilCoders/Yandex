GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    images.go
    models.go
)

GO_TEST_SRCS(models_test.go)

END()

RECURSE(
    compute
    gotest
    porto
)
