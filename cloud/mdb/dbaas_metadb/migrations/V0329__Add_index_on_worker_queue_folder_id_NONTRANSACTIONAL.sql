CREATE INDEX CONCURRENTLY i_worker_queue_folder_id ON dbaas.worker_queue USING HASH (folder_id);

