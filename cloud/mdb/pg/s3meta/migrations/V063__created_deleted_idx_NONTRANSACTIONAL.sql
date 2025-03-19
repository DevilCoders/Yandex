CREATE INDEX CONCURRENTLY IF NOT EXISTS buckets_history_created_deleted_idx
ON s3.buckets_history(created, deleted);
