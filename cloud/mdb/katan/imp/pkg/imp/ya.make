GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    group.go
    imp.go
    tags.go
)

GO_XTEST_SRCS(tags_test.go)

END()

RECURSE(gotest)
