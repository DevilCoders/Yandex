GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cloud_storage.go
    report.go
)

GO_TEST_SRCS(
    cloud_storage_test.go
    s3_report_test.go
)

END()

RECURSE(gotest)
