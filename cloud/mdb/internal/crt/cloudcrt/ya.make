GO_LIBRARY()

OWNER(g:mdb)

#
#GO_TEST_SRCS(
#    existing_cert_test.go
#    issue_cert_test.go
#    parse_cert_test.go
#)

SRCS(
    client.go
    download_cert.go
    errors.go
    existing_pem.go
    issue_pem.go
    list_certs.go
    revoke_cert.go
)

END()

RECURSE(generated)
