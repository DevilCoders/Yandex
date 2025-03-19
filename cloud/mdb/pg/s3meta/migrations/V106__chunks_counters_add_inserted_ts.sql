ALTER TABLE s3.chunks_counters ADD COLUMN IF NOT EXISTS inserted_ts timestamptz;
