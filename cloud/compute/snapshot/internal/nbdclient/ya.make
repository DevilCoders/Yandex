GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    handshake.go
    nbdclient.go
    transmission.go
)

GO_TEST_SRCS(nbd_test.go)

END()

RECURSE(gotest)
