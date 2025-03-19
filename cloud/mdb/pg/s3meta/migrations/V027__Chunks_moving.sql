CREATE TABLE s3.chunks_move_queue (
    bid uuid NOT NULL,
    cid bigint NOT NULL,
    source_shard int NOT NULL,
    dest_shard int NOT NULL,
    queued timestamptz NOT NULL DEFAULT current_timestamp,
    FOREIGN KEY (bid, cid) REFERENCES s3.chunks (bid, cid) ON DELETE CASCADE
);

CREATE INDEX ON s3.chunks_move_queue USING btree (queued);
