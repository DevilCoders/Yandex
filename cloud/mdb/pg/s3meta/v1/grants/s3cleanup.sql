GRANT USAGE ON SCHEMA s3 TO s3cleanup;
GRANT SELECT ON s3.buckets TO s3cleanup;
GRANT SELECT, DELETE ON s3.buckets_history TO s3cleanup;
GRANT SELECT, DELETE ON s3.background_tasks TO s3cleanup;
GRANT SELECT, DELETE ON s3.enabled_lc_rules TO s3cleanup;
GRANT SELECT, DELETE ON s3.lifecycle_scheduling_status TO s3cleanup;
GRANT SELECT, DELETE ON s3.server_logs_tasks TO s3cleanup;
GRANT SELECT, DELETE ON s3.server_logs_tasks_buckets TO s3cleanup;
GRANT SELECT, DELETE ON s3.buckets_usage_processing TO s3cleanup;

GRANT USAGE ON SCHEMA v1_impl TO s3cleanup;
GRANT USAGE ON SCHEMA v1_code TO s3cleanup;

GRANT EXECUTE ON FUNCTION v1_code.bucket_info(text) TO s3cleanup;
