GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    config.go
    pg.go
)

GO_XTEST_SRCS(pg_unit_test.go)

END()

RECURSE(gotest)
