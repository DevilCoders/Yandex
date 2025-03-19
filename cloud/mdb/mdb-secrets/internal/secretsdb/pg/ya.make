GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cert_url.go
    certs.go
    delete_cert.go
    delete_gpg.go
    encrypted_data.go
    get_gpg.go
    put_gpg.go
    service.go
)

GO_TEST_SRCS(encrypted_data_test.go)

END()

RECURSE(gotest)
