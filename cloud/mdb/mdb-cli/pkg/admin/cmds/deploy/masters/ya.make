GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    balance.go
    create.go
    get.go
    list.go
    masters.go
    upsert.go
)

GO_TEST_SRCS(balance_test.go)

END()

RECURSE(gotest)
