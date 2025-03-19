GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(
    factory.go
    generic_rewriter.go
    hdfs_rewriter.go
    hive_rewriter.go
    rewriter.go
    webhdfs_rewriter.go
)

GO_TEST_SRCS(generic_rewriter_test.go)

END()

RECURSE(gotest)
