GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(agentauth.go)

GO_TEST_SRCS(agentauth_test.go)

END()

RECURSE(gotest)
