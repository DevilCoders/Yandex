ALTER TABLE s3.chunks_counters ALTER COLUMN deleted_objects_count SET DEFAULT 0;
ALTER TABLE s3.chunks_counters ALTER COLUMN deleted_objects_size SET DEFAULT 0;
ALTER TABLE s3.chunks_counters ALTER COLUMN active_multipart_count SET DEFAULT 0;
ALTER TABLE s3.chunks_counters ALTER COLUMN storage_class SET DEFAULT 0;
