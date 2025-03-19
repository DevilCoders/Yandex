GRANT CONNECT ON DATABASE dbaas_metadb TO pillar_secrets;
GRANT USAGE ON SCHEMA dbaas TO pillar_secrets;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO pillar_secrets;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO pillar_secrets;
GRANT USAGE ON SCHEMA code TO pillar_secrets;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO pillar_secrets;
