ALTER TABLE snc_snapshots ADD COLUMN IF NOT EXISTS snapshot_context json;
ALTER TABLE snc_snapshots_history ADD COLUMN IF NOT EXISTS snapshot_context json;
