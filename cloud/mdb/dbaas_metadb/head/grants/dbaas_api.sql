GRANT USAGE ON SCHEMA dbaas TO dbaas_api;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO dbaas_api;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO dbaas_api;
GRANT USAGE ON SCHEMA code TO dbaas_api;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO dbaas_api;
