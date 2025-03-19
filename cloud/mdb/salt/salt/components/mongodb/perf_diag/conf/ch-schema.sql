CREATE TABLE IF NOT EXISTS perfdiag.mongodb_profiler_part ON CLUSTER 'active_shards'
(
    ts DateTime CODEC(DoubleDelta, LZ4),
    cluster_id LowCardinality(String),
    shard LowCardinality(String),
    hostname LowCardinality(String),
    user String,
    ns String,
    database String,
    collection String,
    op LowCardinality(String),
    form_hash String,
    form String CODEC(ZSTD),
    duration UInt32,
    plan_summary String,
    response_length UInt32,
    keys_examined UInt32,
    docs_examined UInt32,
    docs_returned UInt32,
    raw String CODEC(ZSTD),
    
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/perfdiag_mongodb_profiler_part', '{replica}')
PARTITION BY toDate(ts, 'UTC')
ORDER BY (cluster_id, ts)
TTL toDate(ts) + INTERVAL 3 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS perfdiag.mongodb_profiler ON CLUSTER 'active_shards' AS perfdiag.mongodb_profiler_part ENGINE = Distributed('active_shards', perfdiag, mongodb_profiler_part, sipHash64(cluster_id));

CREATE TABLE IF NOT EXISTS perfdiag.mongodb_profiler_unparsed_part ON CLUSTER 'active_shards' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/perfdiag_mongodb_profiler_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS perfdiag.mongodb_profiler_unparsed ON CLUSTER 'active_shards' AS perfdiag.mongodb_profiler_unparsed_part ENGINE = Distributed('active_shards', perfdiag, mongodb_profiler_unparsed_part, sipHash64(_partition));

CREATE OR REPLACE VIEW perfdiag.mongodb_profiler_view ON CLUSTER `{cluster}` AS
SELECT
    ts,
    cluster_id,
    shard,
    trim(TRAILING '::012789' from hostname) AS hostname,
    splitByChar('@', user)[1] AS user,
    ns,
    database,
    collection,
    op,
    form_hash,
    form,
    duration,
    plan_summary,
    response_length,
    keys_examined,
    docs_examined,
    docs_returned,
    1 AS count,
    keys_examined / docs_returned AS keys_examined_per_docs_returned,
    docs_examined / docs_returned AS docs_examined_per_docs_returned
FROM perfdiag.mongodb_profiler
