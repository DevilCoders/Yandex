CREATE TABLE IF NOT EXISTS mdb.postgres_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt32 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    message String,
    user_name String,
    database_name String,
    process_id UInt32,
    connection_from String,
    session_id String,
    session_line_num UInt32,
    command_tag String,
    session_start_time DateTime CODEC(DoubleDelta, LZ4),
    virtual_transaction_id String,
    transaction_id UInt64,
    error_severity LowCardinality(String),
    sql_state_code LowCardinality(String),
    detail String,
    hint String,
    internal_query String,
    internal_query_pos UInt32,
    context String,
    query String CODEC(ZSTD),
    query_pos UInt32,
    location String,
    application_name String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    log_time DateTime CODEC(DoubleDelta, LZ4),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_postgres_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.postgres ON CLUSTER 'active_logs' AS mdb.postgres_part ENGINE = Distributed('active_logs', mdb, postgres_part, sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.pgbouncer_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    text String,
    session_id String,
    db String,
    user String,
    source String,
    pid UInt32,
    level String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    log_time DateTime CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_pgbouncer_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.pgbouncer ON CLUSTER 'active_logs' AS mdb.pgbouncer_part ENGINE = Distributed('active_logs', mdb, pgbouncer_part, sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.odyssey_part ON CLUSTER 'active_logs'
(
    unixtime DateTime CODEC(DoubleDelta, LZ4),
    ms UInt32 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    msg String,
    cid String,
    sid String,
    ctx String,
    db String,
    user String,
    pid UInt32,
    level String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_odyssey_part', '{replica}')
PARTITION BY toDate(unixtime, 'UTC')
ORDER BY (cluster, unixtime, ms)
TTL toDate(unixtime) + INTERVAL 1 MONTH DELETE,
    toDate(unixtime) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.odyssey_original ON CLUSTER 'active_logs' AS mdb.odyssey_part ENGINE = Distributed('active_logs', mdb, odyssey_part, sipHash64(cluster));

/* TM does not support column renames for now, so a view is needed to ensure backwards compatibility */
CREATE VIEW IF NOT EXISTS mdb.odyssey ON CLUSTER 'active_logs' AS SELECT
    unixtime AS timestamp,
    ms AS ms,
    cluster AS cluster,
    hostname AS hostname,
    origin AS origin,
    msg AS text,
    cid AS client_id,
    sid AS server_id,
    ctx AS context,
    db AS db,
    user AS user,
    pid AS pid,
    level AS level,
    insert_time AS insert_time,
    log_format AS log_format
FROM mdb.odyssey_original;


CREATE TABLE IF NOT EXISTS mdb.mongodb_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    message String,
    context String,
    component LowCardinality(String),
    severity LowCardinality(String),
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mongodb_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, origin, timestamp, ms) /* mongos, mongocfg etc */
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mongodb ON CLUSTER 'active_logs' AS mdb.mongodb_part ENGINE = Distributed('active_logs', mdb, mongodb_part, sipHash64(cluster));
CREATE VIEW IF NOT EXISTS mdb.mongod ON CLUSTER 'active_logs' AS SELECT * FROM mdb.mongodb WHERE origin = 'mongod';
CREATE VIEW IF NOT EXISTS mdb.mongos ON CLUSTER 'active_logs' AS SELECT * FROM mdb.mongodb WHERE origin = 'mongos';
CREATE VIEW IF NOT EXISTS mdb.mongocfg ON CLUSTER 'active_logs' AS SELECT * FROM mdb.mongodb WHERE origin = 'mongocfg';


CREATE TABLE IF NOT EXISTS mdb.mongodb_audit_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
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
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mongodb_audit_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, origin, timestamp, ms) /* mongos, mongocfg etc */
TTL toDate(timestamp) + INTERVAL 3 MONTH DELETE, /* Yes! We want store 3 months*/
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mongodb_audit ON CLUSTER 'active_logs' AS mdb.mongodb_audit_part
    ENGINE = Distributed('active_logs', mdb, mongodb_audit_part, sipHash64(cluster));


CREATE TABLE IF NOT EXISTS mdb.clickhouse_part ON CLUSTER 'active_logs'
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
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.clickhouse ON CLUSTER 'active_logs' AS mdb.clickhouse_part ENGINE = Distributed('active_logs', mdb, clickhouse_part, sipHash64(cluster));


CREATE TABLE IF NOT EXISTS mdb.redis_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    message String,
    role LowCardinality(String),
    pid UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_redis_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.redis ON CLUSTER 'active_logs' AS mdb.redis_part ENGINE = Distributed('active_logs', mdb, redis_part, sipHash64(cluster));


CREATE TABLE IF NOT EXISTS mdb.mysql_error_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    message String,
    id String,
    status String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    raw String CODEC(ZSTD),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mysql_error_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mysql_error ON CLUSTER 'active_logs' AS mdb.mysql_error_part ENGINE = Distributed('active_logs', mdb, mysql_error_part, sipHash64(cluster));


CREATE TABLE IF NOT EXISTS mdb.mysql_general_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    id String,
    command String,
    argument String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    raw String CODEC(ZSTD),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mysql_general_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mysql_general ON CLUSTER 'active_logs' AS mdb.mysql_general_part ENGINE = Distributed('active_logs', mdb, mysql_general_part, sipHash64(cluster));


CREATE TABLE IF NOT EXISTS mdb.mysql_slow_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    id String,
    user String,
    schema String,
    query String,
    last_errno UInt32,
    killed UInt32,
    query_time Float64,
    lock_time Float64,
    rows_sent UInt32,
    rows_examined UInt32,
    rows_affected UInt32,
    bytes_sent UInt64,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    raw String CODEC(ZSTD),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mysql_slow_query_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mysql_slow_query ON CLUSTER 'active_logs' AS mdb.mysql_slow_part ENGINE = Distributed('active_logs', mdb, mysql_slow_part, sipHash64(cluster));


CREATE TABLE IF NOT EXISTS mdb.mysql_audit_part ON CLUSTER 'active_logs'
(
    cluster LowCardinality(String),
    command_class String,
    connection_id UInt32,
    connection_type String,
    db String,
    hostname LowCardinality(String),
    origin LowCardinality(String),
    ip String,
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    mysql_version String,
    name String,
    os_login LowCardinality(String),
    os_user LowCardinality(String),
    os_version String,
    priv_user String,
    proxy_user String,
    record String,
    server_id String,
    sqltext String,
    startup_optionsi String,
    status UInt32,
    status_code UInt32,
    user String,
    version LowCardinality(String),
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    raw String CODEC(ZSTD),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mysql_audit_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mysql_audit ON CLUSTER 'active_logs' AS mdb.mysql_audit_part ENGINE = Distributed('active_logs', mdb, mysql_audit_part, sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.kafka_part ON CLUSTER 'active_logs'
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
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.kafka ON CLUSTER 'active_logs' AS mdb.kafka_part ENGINE = Distributed('active_logs', mdb, kafka_part, sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.postgres_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_postgres_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.postgres_unparsed ON CLUSTER 'active_logs' AS mdb.postgres_unparsed_part ENGINE = Distributed('active_logs', mdb, postgres_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.pgbouncer_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_pgbouncer_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.pgbouncer_unparsed ON CLUSTER 'active_logs' AS mdb.pgbouncer_unparsed_part ENGINE = Distributed('active_logs', mdb, pgbouncer_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.odyssey_original_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_odyssey_original_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.odyssey_original_unparsed ON CLUSTER 'active_logs' AS mdb.odyssey_original_unparsed_part ENGINE = Distributed('active_logs', mdb, odyssey_original_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.mongodb_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mongodb_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mongodb_unparsed ON CLUSTER 'active_logs' AS mdb.mongodb_unparsed_part ENGINE = Distributed('active_logs', mdb, mongodb_unparsed_part, sipHash64(_partition));


CREATE TABLE IF NOT EXISTS mdb.mongodb_audit_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mongodb_audit_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 3 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mongodb_audit_unparsed ON CLUSTER 'active_logs' AS mdb.mongodb_audit_unparsed_part
    ENGINE = Distributed('active_logs', mdb, mongodb_audit_unparsed_part, sipHash64(_partition));


CREATE TABLE IF NOT EXISTS mdb.clickhouse_unparsed_part ON CLUSTER 'active_logs' (
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

CREATE TABLE IF NOT EXISTS mdb.clickhouse_unparsed ON CLUSTER 'active_logs' AS mdb.clickhouse_unparsed_part ENGINE = Distributed('active_logs', mdb, clickhouse_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.redis_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_redis_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.redis_unparsed ON CLUSTER 'active_logs' AS mdb.redis_unparsed_part ENGINE = Distributed('active_logs', mdb, redis_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.mysql_error_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mysql_error_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mysql_error_unparsed ON CLUSTER 'active_logs' AS mdb.mysql_error_unparsed_part ENGINE = Distributed('active_logs', mdb, mysql_error_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.mysql_general_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mysql_general_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mysql_general_unparsed ON CLUSTER 'active_logs' AS mdb.mysql_general_unparsed_part ENGINE = Distributed('active_logs', mdb, mysql_general_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.mysql_slow_query_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mysql_slow_query_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mysql_slow_query_unparsed ON CLUSTER 'active_logs' AS mdb.mysql_slow_query_unparsed_part ENGINE = Distributed('active_logs', mdb, mysql_slow_query_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.mysql_audit_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_mysql_audit_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.mysql_audit_unparsed ON CLUSTER 'active_logs' AS mdb.mysql_audit_unparsed_part ENGINE = Distributed('active_logs', mdb, mysql_audit_unparsed_part, sipHash64(_partition));


CREATE TABLE IF NOT EXISTS mdb.elasticsearch_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    level LowCardinality(String),
    component LowCardinality(String),
    message String,
    stacktrace String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    datetime DateTime CODEC(DoubleDelta, LZ4),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_elasticsearch_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.elasticsearch ON CLUSTER 'active_logs' AS mdb.elasticsearch_part ENGINE = Distributed('active_logs', mdb, elasticsearch_part, sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.elasticsearch_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_elasticsearch_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.elasticsearch_unparsed ON CLUSTER 'active_logs' AS mdb.elasticsearch_unparsed_part ENGINE = Distributed('active_logs', mdb, elasticsearch_unparsed_part, sipHash64(_partition));


CREATE TABLE IF NOT EXISTS mdb.kibana_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt16 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    type LowCardinality(String),
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
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_kibana_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.kibana ON CLUSTER 'active_logs' AS mdb.kibana_part ENGINE = Distributed('active_logs', mdb, kibana_part, sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.kibana_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_kibana_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.kibana_unparsed ON CLUSTER 'active_logs' AS mdb.kibana_unparsed_part ENGINE = Distributed('active_logs', mdb, kibana_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.kafka_unparsed_part ON CLUSTER 'active_logs' (
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

CREATE TABLE IF NOT EXISTS mdb.kafka_unparsed ON CLUSTER 'active_logs' AS mdb.kafka_unparsed_part ENGINE = Distributed('active_logs', mdb, kafka_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.greenplum_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt32 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    user_name String,
    database_name String,
    process_id String,
    session_start_time DateTime CODEC(DoubleDelta, LZ4),
    transaction_id UInt64,
    sql_state_code LowCardinality(String),
    internal_query String,
    internal_query_pos UInt32,
    log_format LowCardinality(String),
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    sub_tranx_id String,
    remote_host String,
    file_name String,
    distr_tranx_id String,
    event_severity String,
    event_hint String,
    event_time DateTime CODEC(DoubleDelta, LZ4),
    debug_query_string String,
    gp_session_id String,
    stack_trace String,
    local_tranx_id String,
    error_cursor_pos UInt32,
    gp_command_count String,
    slice_id String,
    event_detail String,
    event_context String,
    event_message String,
    func_name String,
    remote_port String,
    thread_id String,
    file_line UInt32,
    gp_segment String,
    gp_host_type String,
    gp_preferred_role String,
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_greenplum_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, timestamp, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.greenplum ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt32 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    user_name String,
    database_name String,
    process_id String,
    session_start_time DateTime CODEC(DoubleDelta, LZ4),
    transaction_id UInt64,
    sql_state_code LowCardinality(String),
    internal_query String,
    internal_query_pos UInt32,
    log_format LowCardinality(String),
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    sub_tranx_id String,
    remote_host String,
    file_name String,
    distr_tranx_id String,
    event_severity String,
    event_hint String,
    event_time DateTime CODEC(DoubleDelta, LZ4),
    debug_query_string String,
    gp_session_id String,
    stack_trace String,
    local_tranx_id String,
    error_cursor_pos UInt32,
    gp_command_count String,
    slice_id String,
    event_detail String,
    event_context String,
    event_message String,
    func_name String,
    remote_port String,
    thread_id String,
    file_line UInt32,
    gp_segment String,
    gp_host_type String,
    gp_preferred_role String,
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = Distributed('active_logs', 'mdb', 'greenplum_part', sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.greenplum_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_greenplum_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.greenplum_unparsed ON CLUSTER 'active_logs' AS mdb.greenplum_unparsed_part ENGINE = Distributed('active_logs', mdb, greenplum_unparsed_part, sipHash64(_partition));

CREATE TABLE IF NOT EXISTS mdb.greenplum_odyssey_part ON CLUSTER 'active_logs'
(
    timestamp DateTime CODEC(DoubleDelta, LZ4),
    ms UInt32 DEFAULT 0,
    cluster LowCardinality(String),
    hostname LowCardinality(String),
    origin LowCardinality(String),
    text String,
    cluster_id String,
    server_id String,
    context String,
    db String,
    user String,
    pid UInt32,
    level String,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    log_format LowCardinality(String),
    _timestamp DateTime CODEC(DoubleDelta, LZ4),
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    _rest String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_greenplum_odyssey_part', '{replica}')
PARTITION BY toDate(timestamp, 'UTC')
ORDER BY (cluster, unixtime, ms)
TTL toDate(timestamp) + INTERVAL 1 MONTH DELETE,
    toDate(timestamp) + INTERVAL 1 WEEK TO DISK 'object_storage'
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.greenplum_odyssey ON CLUSTER 'active_logs' AS mdb.greenplum_odyssey_part ENGINE = Distributed('active_logs', mdb, greenplum_odyssey_part, sipHash64(cluster));

CREATE TABLE IF NOT EXISTS mdb.greenplum_odyssey_unparsed_part ON CLUSTER 'active_logs' (
    _timestamp DateTime,
    _partition LowCardinality(String),
    _offset UInt64,
    _idx UInt32,
    insert_time DateTime DEFAULT now() CODEC(DoubleDelta, LZ4),
    unparsed_row String CODEC(ZSTD),
    reason String CODEC(ZSTD)
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/mdb_greenplum_odyssey_unparsed_part', '{replica}')
PARTITION BY toDate(_timestamp, 'UTC')
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL toDate(_timestamp) + INTERVAL 1 MONTH DELETE
SETTINGS ttl_only_drop_parts=1;

CREATE TABLE IF NOT EXISTS mdb.greenplum_odyssey_unparsed ON CLUSTER 'active_logs' AS mdb.greenplum_odyssey_unparsed_part ENGINE = Distributed('active_logs', mdb, greenplum_odyssey_unparsed_part, sipHash64(_partition));
