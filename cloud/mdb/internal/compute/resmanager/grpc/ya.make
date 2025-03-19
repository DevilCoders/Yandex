GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    clouds.go
    folders.go
)

GO_XTEST_SRCS(clouds_test.go)

END()

RECURSE(gotest)
