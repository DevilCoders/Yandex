ALTER TABLE s3.parts ADD COLUMN IF NOT EXISTS chunks_counters_queue_updated_ts timestamptz;
