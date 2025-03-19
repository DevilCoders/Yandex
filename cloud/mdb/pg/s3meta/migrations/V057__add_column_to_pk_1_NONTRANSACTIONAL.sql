CREATE UNIQUE INDEX CONCURRENTLY IF NOT EXISTS pk_chunks_counters ON s3.chunks_counters USING btree (bid, cid, storage_class);
