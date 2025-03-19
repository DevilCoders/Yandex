ALTER TABLE s3.chunks_counters
  ADD COLUMN deleted_objects_count  bigint,
  ADD COLUMN deleted_objects_size   bigint,
  ADD COLUMN active_multipart_count bigint,
  ADD COLUMN storage_class          int;
