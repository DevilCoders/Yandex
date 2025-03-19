CREATE TABLE s3.buckets_usage
(
    bid              uuid    NOT NULL,
    storage_class    int,
    byte_secs        bigint NOT NULL,
    size_change      bigint NOT NULL,
    start_ts         timestamp with time zone NOT NULL,
    end_ts           timestamp with time zone NOT NULL
);
CREATE UNIQUE INDEX uk_buckets_usage_ts ON s3.buckets_usage(bid, start_ts, coalesce(storage_class, 0));
