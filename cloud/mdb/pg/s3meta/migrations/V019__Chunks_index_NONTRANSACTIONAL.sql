CREATE INDEX CONCURRENTLY idx_bid_start_key ON s3.chunks
    USING btree (bid, coalesce(start_key, '') DESC);
