GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    image.go
    source.go
)

GO_XTEST_SRCS(source_test.go)

END()

RECURSE(gotest)
