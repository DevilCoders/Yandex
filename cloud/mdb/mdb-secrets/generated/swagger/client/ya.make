GO_LIBRARY()

OWNER(g:mdb)

SRCS(mdb_secrets_client.go)

END()

RECURSE(
    certs
    common
    gpg
)
