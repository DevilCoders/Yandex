GO_LIBRARY()

OWNER(g:mdb)

SRCS(chencoder.go)

GO_XTEST_SRCS(chencoder_test.go)

END()

RECURSE(gotest)
