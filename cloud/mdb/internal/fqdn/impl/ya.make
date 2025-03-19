GO_LIBRARY()

OWNER(g:mdb)

SRCS(converter.go)

GO_XTEST_SRCS(converter_test.go)

END()

RECURSE(gotest)
