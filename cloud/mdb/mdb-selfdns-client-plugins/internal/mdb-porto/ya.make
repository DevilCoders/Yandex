GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    mdb-porto.go
    records.go
)

GO_TEST_SRCS(records_test.go)

END()

RECURSE(gotest)
