GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    issue_cert.go
    revoke_cert.go
    storage.go
)

END()

RECURSE(challenge)
