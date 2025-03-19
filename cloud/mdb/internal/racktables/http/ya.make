GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    get_macro.go
    get_macros_with_owners.go
)

GO_TEST_SRCS(
    get_macro_test.go
    get_macros_with_owners_test.go
)

END()

RECURSE(gotest)
