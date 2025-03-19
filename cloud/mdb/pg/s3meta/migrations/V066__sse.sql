ALTER TABLE ONLY s3.buckets
    ADD COLUMN encryption_settings JSONB;
