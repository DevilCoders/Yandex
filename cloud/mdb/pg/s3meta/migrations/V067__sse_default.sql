ALTER TABLE ONLY s3.buckets
    ALTER COLUMN encryption_settings SET DEFAULT NULL;
