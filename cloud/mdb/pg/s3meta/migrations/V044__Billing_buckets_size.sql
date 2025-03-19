CREATE TABLE s3.buckets_size
(
    bid              uuid    NOT NULL,
    shard_id         int     NOT NULL,
    storage_class    int     NOT NULL,
    size             bigint  NOT NULL,
    target_ts        timestamp with time zone NOT NULL
);
CREATE UNIQUE INDEX ui_buckets_size ON s3.buckets_size(bid, shard_id, storage_class, target_ts);
