GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(resource.go)

END()

RECURSE(
    cc
    test
    test-bin
    test-fileonly
    test-files
    test-keyonly
)
