GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    certs_client.go
    get_certificate_parameters.go
    get_certificate_responses.go
    put_parameters.go
    put_responses.go
    revoke_certificate_parameters.go
    revoke_certificate_responses.go
)

END()
