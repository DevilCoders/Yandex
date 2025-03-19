ALTER TABLE s3.parts ADD COLUMN IF NOT EXISTS last_counters_updated_ts timestamptz;
