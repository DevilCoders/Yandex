ALTER TABLE s3.objects
    ADD COLUMN lock_settings JSONB DEFAULT NULL;

ALTER TABLE s3.objects_noncurrent
    ADD COLUMN lock_settings JSONB DEFAULT NULL;

ALTER TABLE s3.object_parts
    ADD COLUMN lock_settings JSONB DEFAULT NULL;
