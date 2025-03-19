UPDATE dbaas.worker_queue
SET created_by = ''
WHERE created_by IS NULL;
