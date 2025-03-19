CREATE INDEX CONCURRENTLY ON s3.storage_delete_queue (deleted_ts DESC) WHERE storage_class = 2;
