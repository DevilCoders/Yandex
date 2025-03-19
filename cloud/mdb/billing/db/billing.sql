CREATE SCHEMA billing;

CREATE TYPE billing.bill_type AS ENUM (
    'BACKUP',
    'CH_CLOUD_STORAGE'
);

CREATE TYPE billing.cluster_type AS ENUM (
    'postgresql_cluster',
    'zookeeper',
    'clickhouse_cluster',
    'mongodb_cluster',
    'redis_cluster',
    'mysql_cluster',
    'sqlserver_cluster',
    'greenplum_cluster',
    'hadoop_cluster',
    'kafka_cluster',
    'elasticsearch_cluster'
);

CREATE TABLE billing.tracks (
    cluster_id    text NOT NULL,
    cluster_type  billing.cluster_type NOT NULL,
    bill_type     billing.bill_type NOT NULL,
    from_ts       timestamptz NOT NULL,
    until_ts      timestamptz,
    updated_at    timestamptz,

    CONSTRAINT pk_tracks PRIMARY KEY (cluster_id, bill_type),
    CONSTRAINT check_from_ts_less_then_until_ts CHECK (from_ts < until_ts)
);


CREATE TABLE billing.metrics_queue (
    batch_id         uuid NOT NULL,
    bill_type        billing.bill_type NOT NULL,
    created_at       timestamptz NOT NULL DEFAULT now(),
    started_at       timestamptz,
    finished_at      timestamptz,
    updated_at       timestamptz,
    restart_count    bigint DEFAULT 0 NOT NULL,
    seq_no           bigint NOT NULL,
    batch            bytea,

    CONSTRAINT pk_metrics_queue PRIMARY KEY (batch_id)
);

CREATE UNIQUE INDEX uk_bill_type_seq_no ON billing.metrics_queue (bill_type, seq_no);
