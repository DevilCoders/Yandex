CREATE DATABASE mdb ON CLUSTER '{cluster}';

--///////////////////////////////////////////////////////
--//                Table schema                       //
--///////////////////////////////////////////////////////

CREATE TABLE IF NOT EXISTS mdb.clickhouse_part ON CLUSTER '{cluster}'
(
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    message String,
    component String,
    thread UInt32,
    severity LowCardinality(String),
    query_id String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_clickhouse_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.clickhouse ON CLUSTER '{cluster}' AS mdb.clickhouse_part ENGINE = Distributed('{cluster}', mdb, clickhouse_part, sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.kafka_part ON CLUSTER '{cluster}'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    severity LowCardinality(String),
    message String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_kafka_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.kafka ON CLUSTER '{cluster}' AS mdb.kafka_part ENGINE = Distributed('{cluster}', mdb, kafka_part, sipHash64(cluster));

-- Unparsed log tables

CREATE TABLE IF NOT EXISTS mdb.clickhouse_unparsed_part ON CLUSTER '{cluster}' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_clickhouse_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.clickhouse_unparsed ON CLUSTER '{cluster}' AS mdb.clickhouse_unparsed_part ENGINE = Distributed('{cluster}', mdb, clickhouse_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.kafka_unparsed_part ON CLUSTER '{cluster}' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_kafka_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.kafka_unparsed ON CLUSTER '{cluster}' AS mdb.kafka_unparsed_part ENGINE = Distributed('{cluster}', mdb, kafka_unparsed_part, sipHash64(_partition));

-- DATA TRANSFER

CREATE TABLE IF NOT EXISTS mdb.data_transfer_dp ON CLUSTER '{cluster}' (
    `_timestamp` DateTime64,
    `id` Nullable(String),
    `task_id` Nullable(String),
    `level` Nullable(String),
    `msg` Nullable(String),
    `caller` Nullable(String),
    `error` Nullable(String),
    `runtime` Nullable(String),
    `host` Nullable(String),
    `job_index` Nullable(Int32),
    `ts` Nullable(String),
    `_partition` LowCardinality(String),
    `_offset` UInt64,
    `_idx` UInt32
)
    ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_data_transfer_dp', '{replica}')
    PARTITION BY toDate(_timestamp, 'UTC')
    ORDER BY (id, _timestamp, _partition, _offset, _idx)
    TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
    SETTINGS allow_nullable_key = 1, index_granularity = 8192;


CREATE TABLE IF NOT EXISTS mdb.data_transfer_dp_d ON CLUSTER '{cluster}' AS mdb.data_transfer_dp ENGINE = Distributed('{cluster}', mdb, data_transfer_dp, sipHash64(_partition))