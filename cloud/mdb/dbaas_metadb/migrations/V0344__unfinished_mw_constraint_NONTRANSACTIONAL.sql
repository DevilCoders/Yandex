CREATE UNIQUE INDEX CONCURRENTLY uk_worker_queue_single_mw_config_task
    ON dbaas.worker_queue (cid, config_id)
    WHERE result IS NULL
    AND config_id IS NOT NULL;
