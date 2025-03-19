GO_PROGRAM()

OWNER(g:cloud-nbs)

SRCS(
    encrypt.go
    s3.go
    yc-snapshot-backup.go
    ydb.go
    ydb_util.go
)

GO_TEST_SRCS(encrypt_test.go)

END()

RECURSE(gotest)
