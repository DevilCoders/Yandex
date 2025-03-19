CREATE INDEX CONCURRENTLY i_search_queue_queue_id
    ON dbaas.search_queue USING HASH (queue_id);
