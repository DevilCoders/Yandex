GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    gitd.go
    mariadb_gtid.go
    mysql_gtid.go
    wrapper.go
)

GO_TEST_SRCS(
    mariadb_gtid_test.go
    mysql_gtid_test.go
)

END()

RECURSE(gotest)
