GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    import_vpc.go
    peering.go
    validator.go
)

GO_XTEST_SRCS(
    import_vpc_test.go
    peering_test.go
)

END()

RECURSE(gotest)
