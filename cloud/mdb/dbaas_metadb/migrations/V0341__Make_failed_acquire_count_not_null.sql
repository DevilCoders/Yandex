UPDATE dbaas.worker_queue SET failed_acquire_count = 0 WHERE failed_acquire_count IS NULL;
ALTER TABLE dbaas.worker_queue ALTER COLUMN failed_acquire_count SET NOT NULL;
