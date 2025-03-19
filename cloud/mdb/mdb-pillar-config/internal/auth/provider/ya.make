GO_LIBRARY()

OWNER(g:mdb)

SRCS(metadbauthenticator.go)

GO_TEST_SRCS(metadbauthenticator_test.go)

END()

RECURSE(gotest)
