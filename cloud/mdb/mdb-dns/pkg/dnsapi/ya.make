GO_LIBRARY()

OWNER(g:mdb)

SRCS(dnsapi.go)

GO_XTEST_SRCS(dnsapi_test.go)

END()

RECURSE(
    compute
    gotest
    http
    mem
    route53
)
