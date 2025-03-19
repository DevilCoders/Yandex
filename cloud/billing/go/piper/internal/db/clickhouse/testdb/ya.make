GO_LIBRARY()

SRCS(
    db.go
    sql.go
)

GO_TEST_SRCS(db_test.go)

END()

RECURSE(gotest)
