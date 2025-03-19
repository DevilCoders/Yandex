-- All buckets with max_size = NULL are considered as "unlimited"
ALTER TABLE ONLY s3.buckets ADD COLUMN
    max_size  bigint;
