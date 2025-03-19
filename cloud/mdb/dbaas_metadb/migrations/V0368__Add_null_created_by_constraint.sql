ALTER TABLE dbaas.worker_queue
    ADD CONSTRAINT check_worker_task_created_by_not_null CHECK (created_by IS NOT NULL) NOT VALID;

ALTER TABLE dbaas.worker_queue
    ALTER COLUMN created_by SET DEFAULT '';
