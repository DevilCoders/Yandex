CREATE TABLE autocheck.metrics_local ON CLUSTER 'mdbg9qacebrj79jiuhat'
(
    branch       String Codec (LZ4),
    path         String Codec (LZ4),
    test_name    String Codec (LZ4),
    subtest_name String Codec (LZ4),
    toolchain    String Codec (LZ4),
    metric_name  String Codec (LZ4),
    date         DateTime,
    revision     UInt64,
    result_type  String Codec (LZ4),
    value        Float64,
    test_status  String Codec (LZ4),
    check_id     UInt64,
    suite_id     UInt64 Codec (Delta(8), LZ4),
    test_id      UInt64 Codec (Delta(8), LZ4)
) ENGINE = ReplicatedMergeTree
      PARTITION BY toMonth(date)
      ORDER BY (
                branch,
                path,
                test_name,
                subtest_name,
                toolchain,
                metric_name,
                date,
                revision
          )
      TTL date + INTERVAL 14 DAY TO DISK 'object_storage'
    SETTINGS index_granularity = 8192;

CREATE TABLE autocheck.metrics ON CLUSTER 'mdbg9qacebrj79jiuhat' AS autocheck.metrics_local
ENGINE = Distributed('mdbg9qacebrj79jiuhat', autocheck, metrics_local, rand())
