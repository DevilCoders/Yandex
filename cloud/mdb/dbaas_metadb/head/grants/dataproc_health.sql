GRANT CONNECT ON DATABASE dbaas_metadb TO dataproc_health;
GRANT USAGE ON SCHEMA dbaas TO dataproc_health;
GRANT SELECT ON TABLE dbaas.clusters TO dataproc_health;
