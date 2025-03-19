CREATE DATABASE health;

CREATE TABLE health.host_health
(
    cluster_id     String,
    fqdn           String,
    created        DateTime,
    status         LowCardinality(String),
    expired        DateTime,
    services Nested
        (
        name LowCardinality(String),
        service_time DateTime,
        status LowCardinality(String),
        role LowCardinality(String),
        replica_type LowCardinality(String),
        replica_upstream LowCardinality(String),
        replica_lag Int64,
        metrics String
        ),
    system_metrics Nested
        (
        metric_type LowCardinality(String),
        metric_time DateTime,
        used Float64,
        total Float64
        ),
    mode           Array(UInt8),
    mode_timestamp DateTime
) ENGINE MergeTree()
      ORDER BY (cluster_id, fqdn, created)
      PARTITION BY toYYYYMMDD(created);
