GO_LIBRARY()

OWNER(
    gzuykov
    buglloc
    g:go-library
)

SRCS(resolver.go)

END()

RECURSE(
    adaptive
    cached
    golist
    gopackages
)
