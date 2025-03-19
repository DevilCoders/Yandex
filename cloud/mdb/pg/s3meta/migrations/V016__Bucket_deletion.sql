CREATE TABLE s3.chunks_delete_queue (
    bid uuid NOT NULL,
    cid bigint NOT NULL,
    bucket_name text NOT NULL,
    created timestamptz NOT NULL,
    start_key text COLLATE "C",
    end_key text COLLATE "C",
    shard_id int NOT NULL,
    queued timestamptz NOT NULL DEFAULT current_timestamp,
    CONSTRAINT pk_chunks_delete PRIMARY KEY (bid, cid)
);

CREATE INDEX ON s3.chunks_delete_queue USING btree (queued);

ALTER TABLE s3.chunks DROP CONSTRAINT fk_chunks_bid_buckets;
ALTER TABLE s3.chunks ADD CONSTRAINT fk_chunks_bid_buckets
    FOREIGN KEY (bid) REFERENCES s3.buckets ON DELETE RESTRICT;
