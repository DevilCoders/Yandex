GRANT USAGE ON SCHEMA dbaas TO dbaas_support;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO dbaas_support;

GRANT INSERT, UPDATE, DELETE ON dbaas.clouds TO dbaas_support;
GRANT INSERT, UPDATE, DELETE ON dbaas.folders TO dbaas_support;
GRANT UPDATE ON dbaas.clusters TO dbaas_support;

GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO dbaas_support;
GRANT USAGE ON SCHEMA code TO dbaas_support;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO dbaas_support;
