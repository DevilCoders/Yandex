CREATE TABLE s3.buckets_usage_tmpl
(
    bid              uuid    NOT NULL,
    shard_id         int     NOT NULL,
    storage_class    int     NOT NULL,
    byte_secs        bigint  NOT NULL,
    size_change      bigint  NOT NULL,
    start_ts         timestamp with time zone NOT NULL,
    end_ts           timestamp with time zone NOT NULL
);
CREATE UNIQUE INDEX uk_buckets_usage_tmpl_ts ON s3.buckets_usage_tmpl(bid, shard_id, storage_class, start_ts);


CREATE TABLE s3.buckets_usage
(
    bid              uuid    NOT NULL,
    shard_id         int     NOT NULL,
    storage_class    int     NOT NULL,
    byte_secs        bigint  NOT NULL,
    size_change      bigint  NOT NULL,
    start_ts         timestamp with time zone NOT NULL,
    end_ts           timestamp with time zone NOT NULL
) PARTITION BY RANGE (start_ts);

CREATE SCHEMA IF NOT EXISTS partman;
CREATE EXTENSION IF NOT EXISTS pg_partman SCHEMA partman;
SELECT partman.create_parent('s3.buckets_usage', 'start_ts', 'native', 'daily', NULL, 4, p_template_table := 's3.buckets_usage_tmpl', p_jobmon := false);
UPDATE partman.part_config SET retention = '30 days' WHERE parent_table = 's3.buckets_usage';
