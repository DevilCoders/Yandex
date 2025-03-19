ALTER TABLE s3.chunks_counters ALTER COLUMN deleted_objects_count SET NOT NULL;
ALTER TABLE s3.chunks_counters ALTER COLUMN deleted_objects_size SET NOT NULL;
ALTER TABLE s3.chunks_counters ALTER COLUMN active_multipart_count SET NOT NULL;
ALTER TABLE s3.chunks_counters ALTER COLUMN storage_class SET NOT NULL;
