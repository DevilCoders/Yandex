GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cms.go
    filter.go
)

GO_TEST_SRCS(filter_test.go)

END()

RECURSE(gotest)
