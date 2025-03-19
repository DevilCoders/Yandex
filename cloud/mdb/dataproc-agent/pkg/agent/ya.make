GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(
    agent.go
    app.go
    hbase_status.go
    hdfs_status.go
    hive_status.go
    livy_status.go
    metrics_server.go
    oozie_status.go
    yarn_cluster_metrics.go
    yarn_status.go
    zk_status.go
)

GO_TEST_SRCS(
    hbase_status_test.go
    hdfs_status_test.go
    hive_status_test.go
    metrics_server_test.go
    oozie_status_test.go
    yarn_cluster_metrics_test.go
    yarn_status_test.go
)

END()

RECURSE(gotest)
