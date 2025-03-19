ALTER TABLE s3.buckets_history
    ADD COLUMN yc_tags JSONB DEFAULT NULL;

UPDATE s3.buckets_history h
    SET yc_tags = b.yc_tags
    FROM s3.buckets b
    WHERE h.bid = b.bid;
