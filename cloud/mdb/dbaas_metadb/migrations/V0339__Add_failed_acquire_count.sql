ALTER TABLE dbaas.worker_queue ADD COLUMN failed_acquire_count bigint;
ALTER TABLE dbaas.worker_queue ALTER COLUMN failed_acquire_count SET DEFAULT 0;
