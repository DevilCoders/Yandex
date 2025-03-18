GO_LIBRARY()

OWNER(g:go-library)

SRCS(encoding.go)

END()

RECURSE(
    protoseq
    unknownjson
)
