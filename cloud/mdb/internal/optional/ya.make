GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    intervals.go
    optional.go
    structs.go
)

GO_XTEST_SRCS(
    optional_test.go
    structs_test.go
)

END()

RECURSE(gotest)
