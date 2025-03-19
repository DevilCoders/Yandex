CREATE INDEX CONCURRENTLY IF NOT EXISTS bucket_usage_sorted_queue_idx
ON s3.buckets_usage_processing(status, start_ts);
