GRANT USAGE ON SCHEMA deploy TO deploy_cleaner;
GRANT SELECT, UPDATE, DELETE ON ALL TABLES IN SCHEMA deploy TO deploy_cleaner;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA deploy TO deploy_cleaner;
GRANT USAGE ON SCHEMA code TO deploy_cleaner;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO deploy_cleaner;
GRANT SELECT ON TABLE public.schema_version TO deploy_cleaner;
GRANT CONNECT ON DATABASE deploydb TO deploy_cleaner;
