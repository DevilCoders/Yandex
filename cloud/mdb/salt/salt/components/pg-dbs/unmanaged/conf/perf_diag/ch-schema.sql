CREATE TABLE IF NOT EXISTS perf_diag.pg_stat_activity
(
    `_timestamp` DateTime,
    `_partition` String,
    `_offset` UInt64,
    `_idx` UInt32,
    `collect_time` DateTime,
    `cluster_id` LowCardinality(String),
    `host` LowCardinality(String),
    `xact_start` Nullable(DateTime),
    `database` LowCardinality(String),
    `pid` Nullable(UInt32),
    `user` LowCardinality(String),
    `application_name` Nullable(String),
    `client_addr` Nullable(String),
    `client_hostname` Nullable(String),
    `client_port` Nullable(UInt32),
    `backend_start` Nullable(DateTime),
    `cluster_name` LowCardinality(String),
    `query_start` Nullable(DateTime),
    `state_change` Nullable(DateTime),
    `wait_event_type` LowCardinality(String),
    `wait_event` LowCardinality(String),
    `state` LowCardinality(String),
    `backend_xid` Nullable(UInt32),
    `backend_xmin` Nullable(UInt32),
    `query` Nullable(String) CODEC(ZSTD(7)),
    `backend_type` LowCardinality(String),
    `blocking_pids` Nullable(String),
    `queryid` Nullable(String),
    `_rest` Nullable(String)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/pg_stat_activity_cdc1', '{replica}')
PARTITION BY toDate(collect_time, 'UTC')
ORDER BY (cluster_id, host, collect_time)
TTL toDate(collect_time, 'UTC') + toIntervalDay(14)
SETTINGS index_granularity = 8192;

CREATE TABLE IF NOT EXISTS perf_diag.pg_stat_statements
(
    `_timestamp` DateTime,
    `_partition` String,
    `_offset` UInt64,
    `_idx` UInt32,
    `queryid` String,
    `database` LowCardinality(String),
    `user` LowCardinality(String),
    `host` LowCardinality(String),
    `cluster_id` LowCardinality(String),
    `collect_time` DateTime,
    `shared_blks_dirtied` UInt64,
    `total_time` Float64,
    `min_time` Float64,
    `max_time` Float64,
    `mean_time` Float64,
    `stddev_time` Float64,
    `rows` UInt64,
    `shared_blks_hit` UInt64,
    `shared_blks_read` UInt64,
    `calls` UInt64,
    `shared_blks_written` UInt64,
    `local_blks_hit` UInt64,
    `local_blks_read` UInt64,
    `local_blks_dirtied` UInt64,
    `local_blks_written` UInt64,
    `temp_blks_read` UInt64,
    `temp_blks_written` UInt64,
    `blk_read_time` Float64,
    `blk_write_time` Float64,
    `reads` UInt64,
    `writes` UInt64,
    `user_time` Float64,
    `system_time` Float64,
    `_rest` String,
    `query` String CODEC(ZSTD(7))
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/pg_stat_statements_cdc', '{replica}')
PARTITION BY toDate(collect_time, 'UTC')
ORDER BY (cluster_id, host, database, user, queryid, collect_time)
TTL toDate(collect_time, 'UTC') + toIntervalDay(14)
SETTINGS index_granularity = 8192;

CREATE TABLE IF NOT EXISTS perf_diag.my_sessions
(
    `_timestamp` DateTime,
    `_partition` String,
    `_offset` UInt64,
    `_idx` UInt32,
    `host` String,
    `cluster_id` String,
    `collect_time` DateTime,
    `lock_latency` Nullable(Float64),
    `user` Nullable(String),
    `thd_id` Nullable(UInt32),
    `conn_id` Nullable(UInt32),
    `command` Nullable(String),
    `query` Nullable(String),
    `digest` Nullable(String),
    `query_latency` Nullable(Float64),
    `database` Nullable(String),
    `stage` Nullable(String),
    `stage_latency` Nullable(Float64),
    `current_wait` Nullable(String),
    `wait_object` Nullable(String),
    `wait_latency` Nullable(Float64),
    `trx_latency` Nullable(Float64),
    `current_memory` Nullable(UInt64),
    `client_addr` Nullable(String),
    `client_hostname` Nullable(String),
    `client_port` Nullable(UInt32),
    `_rest` Nullable(String)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/perf_diag.my_sessions_cdc_new', '{replica}')
PARTITION BY toDate(collect_time, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx, host, cluster_id, collect_time)
TTL toDate(collect_time, 'UTC') + toIntervalDay(14)
SETTINGS index_granularity = 8192;

CREATE TABLE IF NOT EXISTS perf_diag.my_statements
(
    `_timestamp` DateTime,
    `_partition` String,
    `_offset` UInt64,
    `_idx` UInt32,
    `digest` String,
    `database` String,
    `host` String,
    `cluster_id` String,
    `collect_time` DateTime,
    `rows_affected` Nullable(UInt64),
    `total_query_latency` Nullable(Float64),
    `total_lock_latency` Nullable(Float64),
    `avg_query_latency` Nullable(Float64),
    `avg_lock_latency` Nullable(Float64),
    `calls` Nullable(UInt64),
    `errors` Nullable(UInt64),
    `warnings` Nullable(UInt64),
    `rows_examined` Nullable(UInt64),
    `rows_sent` Nullable(UInt64),
    `query` Nullable(String),
    `tmp_tables` Nullable(UInt64),
    `tmp_disk_tables` Nullable(UInt64),
    `select_full_join` Nullable(UInt64),
    `select_full_range_join` Nullable(UInt64),
    `select_range` Nullable(UInt64),
    `select_range_check` Nullable(UInt64),
    `select_scan` Nullable(UInt64),
    `sort_merge_passes` Nullable(UInt64),
    `sort_range` Nullable(UInt64),
    `sort_rows` Nullable(UInt64),
    `sort_scan` Nullable(UInt64),
    `no_index_used` Nullable(UInt64),
    `no_good_index_used` Nullable(UInt64),
    `_rest` Nullable(String)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/perf_diag.my_statements_cdc_new', '{replica}')
PARTITION BY toDate(collect_time, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx, digest, database, host, cluster_id, collect_time)
TTL toDate(collect_time, 'UTC') + toIntervalDay(14)
SETTINGS index_granularity = 8192;
