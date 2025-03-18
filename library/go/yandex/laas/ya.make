GO_LIBRARY()

OWNER(
    gzuykov
    g:go-library
)

SRCS(client.go)

END()

RECURSE(
    httplaas
    mocklaas
)
