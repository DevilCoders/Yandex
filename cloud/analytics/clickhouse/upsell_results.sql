CREATE TABLE %(table_name)s
(
    experiment_index Int64,
    experiment_name String,
    pvalue Double,
    test_conversion_pct Double,
    control_conversion_pct Double,
    test_avg_consumption Double,
    control_avg_consumption Double,
    significant_difference String,
    date String
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
PARTITION BY experiment_index
ORDER BY experiment_index
SAMPLE BY experiment_index
SETTINGS index_granularity = 8192
