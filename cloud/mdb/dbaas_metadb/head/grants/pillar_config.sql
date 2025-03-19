GRANT CONNECT ON DATABASE dbaas_metadb TO pillar_config;
GRANT USAGE ON SCHEMA dbaas TO pillar_config;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO pillar_config;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO pillar_config;
GRANT USAGE ON SCHEMA code TO pillar_config;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO pillar_config;
