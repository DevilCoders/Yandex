ALTER TABLE ONLY s3.buckets
    ADD COLUMN website_settings JSONB DEFAULT NULL,
    ADD COLUMN cors_configuration JSONB DEFAULT NULL,
    ADD COLUMN default_storage_class int DEFAULT NULL,
    ADD COLUMN yc_tags JSONB DEFAULT NULL,
    ADD COLUMN lifecycle_rules JSONB DEFAULT NULL,
    ADD COLUMN system_settings JSONB DEFAULT NULL;