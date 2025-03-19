GO_LIBRARY()

OWNER(g:mdb)

SRCS(dns.go)

GO_TEST_SRCS(dns_test.go)

END()

RECURSE(gotest)
