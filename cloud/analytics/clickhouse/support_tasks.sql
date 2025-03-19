CREATE TABLE %(table_name)s
(
    response_time Nullable(Int32),
    response_time_group Nullable(String),
    task Nullable(String),
    task_time DateTime,
    task_type Nullable(String),
    in_time_response Nullable(String),
    author Nullable(String),
    pay_type Nullable(String),
    task_link Nullable(String)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
ORDER BY(task_time) PARTITION BY toYYYYMM(task_time)