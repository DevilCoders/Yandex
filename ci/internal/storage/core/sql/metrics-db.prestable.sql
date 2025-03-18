CREATE TABLE autocheck.metrics_local ON CLUSTER 'mdbh6g2i66n6vq81uq2n'
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

CREATE TABLE autocheck.metrics ON CLUSTER 'mdbh6g2i66n6vq81uq2n' AS autocheck.metrics_local
ENGINE = Distributed('mdbh6g2i66n6vq81uq2n', autocheck, metrics_local, rand())
