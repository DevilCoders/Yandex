GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    bycntrs.go
    healthiness.go
)

GO_TEST_SRCS(bycntrs_test.go)

END()

RECURSE(
    gotest
    healthdbspec
)
