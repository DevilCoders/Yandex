GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    parse_cert.go
)

GO_TEST_SRCS(parse_cert_test.go)

END()

RECURSE(
    cloudcrt
    fixtures
    gotest
    letsencrypt
    mocks
    yacrt
)
