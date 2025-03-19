GO_LIBRARY()

OWNER(g:mdb)

SRCS(porto.go)

GO_TEST_SRCS(porto_test.go)

END()

RECURSE(gotest)
