-- All buckets with service_id = NULL are considered as "publicly accessible"
ALTER TABLE ONLY s3.buckets ADD COLUMN
    service_id  bigint;
