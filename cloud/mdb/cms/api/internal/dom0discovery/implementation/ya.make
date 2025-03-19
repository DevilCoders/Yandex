GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    dom0instances.go
    implementation.go
)

GO_TEST_SRCS(dom0instances_test.go)

END()

RECURSE(gotest)
