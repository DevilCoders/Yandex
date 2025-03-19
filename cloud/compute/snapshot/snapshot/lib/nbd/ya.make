GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    errors.go
    nbd.go
    nbd_connector.go
)

GO_TEST_SRCS(
    nbd_connector_test.go
    nbd_test.go
)

END()

RECURSE(gotest)
