UPDATE s3.chunks_counters SET deleted_objects_count = 0, deleted_objects_size = 0
  WHERE (deleted_objects_count IS NULL) OR (deleted_objects_size IS NULL);
UPDATE s3.chunks_counters SET active_multipart_count = 0
  WHERE active_multipart_count IS NULL;
UPDATE s3.chunks_counters SET storage_class = 0
  WHERE storage_class IS NULL;
