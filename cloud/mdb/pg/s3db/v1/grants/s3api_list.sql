GRANT USAGE ON SCHEMA s3 TO s3api_list;
GRANT SELECT ON ALL SEQUENCES IN SCHEMA s3 TO s3api_list;
GRANT SELECT ON ALL TABLES IN SCHEMA s3 TO s3api_list;

GRANT USAGE ON SCHEMA v1_code TO s3api_list;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA v1_code TO s3api_list;

GRANT USAGE ON SCHEMA v1_impl TO s3api_list;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA v1_impl TO s3api_list;
