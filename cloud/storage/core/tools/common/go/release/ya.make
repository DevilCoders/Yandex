GO_LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
)

SRCS(
    release.go
)

GO_TEST_SRCS(
    release_test.go
)

GO_EMBED_PATTERN(restart.template)

END()

RECURSE_FOR_TESTS(gotest)
