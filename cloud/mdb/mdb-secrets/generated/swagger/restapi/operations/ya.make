GO_LIBRARY()

OWNER(g:mdb)

SRCS(mdb_secrets_api.go)

END()

RECURSE(
    certs
    common
    gpg
)
