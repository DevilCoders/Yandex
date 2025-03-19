GO_LIBRARY()

OWNER(g:mdb)

SRCS(models.go)

GO_XTEST_SRCS(models_test.go)

END()

RECURSE(gotest)
