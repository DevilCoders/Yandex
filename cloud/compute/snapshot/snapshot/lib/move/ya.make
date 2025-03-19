GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    device.go
    nbd.go
    nbs.go
    null.go
    snapshot.go
    url.go
)

GO_TEST_SRCS(nbs_test.go)

END()

RECURSE(gotest)
