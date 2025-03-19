GO_LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
)

SRCS(
    durable.go
    pssh.go
)

GO_TEST_SRCS(
    pssh_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
