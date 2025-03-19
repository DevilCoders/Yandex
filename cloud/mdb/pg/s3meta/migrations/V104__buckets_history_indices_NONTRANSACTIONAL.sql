CREATE INDEX CONCURRENTLY IF NOT EXISTS buckets_updated_ts_idx ON s3.buckets(updated_ts);
CREATE INDEX CONCURRENTLY IF NOT EXISTS buckets_history_deleted_idx ON s3.buckets_history(deleted);
DROP INDEX CONCURRENTLY IF EXISTS buckets_history_created_idx;
