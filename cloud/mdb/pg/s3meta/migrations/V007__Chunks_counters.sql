CREATE TABLE s3.chunks_counters (
    bid uuid NOT NULL,
    cid bigint NOT NULL,
    simple_objects_count bigint NOT NULL,
    simple_objects_size bigint NOT NULL,
    multipart_objects_count bigint NOT NULL,
    multipart_objects_size bigint NOT NULL,
    deleted_objects_count bigint NOT NULL,
    deleted_objects_size bigint NOT NULL,
    objects_parts_count bigint NOT NULL,
    objects_parts_size bigint NOT NULL,
    deleted_objects_parts_count bigint NOT NULL,
    deleted_objects_parts_size bigint NOT NULL,
    updated_ts timestamptz NOT NULL,
    PRIMARY KEY (bid, cid),
    CONSTRAINT fk_objects_bid_cid_chunks FOREIGN KEY (bid, cid)
        REFERENCES s3.chunks ON DELETE CASCADE,
    CONSTRAINT check_counters CHECK (
       simple_objects_count >= 0 AND simple_objects_size >= 0
       AND multipart_objects_count >= 0 AND multipart_objects_size >= 0
       AND deleted_objects_count >= 0 AND deleted_objects_size >= 0
       AND objects_parts_count >= 0 AND objects_parts_size >= 0
       AND deleted_objects_parts_count >= 0 AND deleted_objects_parts_size >= 0
    )
);
