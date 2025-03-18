GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(buildinfo.go)

END()

RECURSE(test)
