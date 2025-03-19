GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    config.go
    keys.go
    plaintext.go
)

END()

RECURSE(aws)
