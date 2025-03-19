-- Consider all buckets as "private" by default
ALTER TABLE ONLY s3.buckets
    ADD COLUMN anonymous_read boolean NOT NULL DEFAULT FALSE,
    ADD COLUMN anonymous_list boolean NOT NULL DEFAULT FALSE;
