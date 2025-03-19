GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    fields.go
    versioned.go
)

GO_TEST_SRCS(
    fields_test.go
    versioned_test.go
)

END()

RECURSE(gotest)
