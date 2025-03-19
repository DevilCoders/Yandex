GO_LIBRARY()

OWNER(g:cloud-nbs)

PEERDIR(
)

SRCS(
    walle.go
)

GO_TEST_SRCS(
    walle_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
