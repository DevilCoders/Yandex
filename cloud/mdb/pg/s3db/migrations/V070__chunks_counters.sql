CREATE TABLE s3.chunks_counters
(
    bid                     uuid    NOT NULL,
    cid                     bigint  NOT NULL,
    storage_class           int     NOT NULL,
    simple_objects_count    bigint  NOT NULL,
    simple_objects_size     bigint  NOT NULL,
    multipart_objects_count bigint  NOT NULL,
    multipart_objects_size  bigint  NOT NULL,
    objects_parts_count     bigint  NOT NULL,
    objects_parts_size      bigint  NOT NULL,
    deleted_objects_count   bigint  NOT NULL,
    deleted_objects_size    bigint  NOT NULL,
    active_multipart_count  bigint  NOT NULL,
    updated_ts              timestamp with time zone NOT NULL DEFAULT current_timestamp
);
CREATE UNIQUE INDEX uk_chunks_counters ON s3.chunks_counters(bid, cid, storage_class);
