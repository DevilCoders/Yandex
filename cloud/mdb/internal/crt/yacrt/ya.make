GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    existing_cert.go
    issue_cert.go
    list_certs.go
    revoke_cert.go
    round_tripper_mock.go
)

GO_TEST_SRCS(
    client_test.go
    existing_cert_test.go
    issue_cert_test.go
    revoke_cert_test.go
)

END()

RECURSE(gotest)
