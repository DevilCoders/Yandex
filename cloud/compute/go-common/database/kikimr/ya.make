GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.go
    kikimr.go
)

GO_TEST_SRCS(z_test.go)

END()

RECURSE(gotest)
