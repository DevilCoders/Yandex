GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(jar.go)

GO_TEST_SRCS(jar_test.go)

END()

RECURSE(gotest)
