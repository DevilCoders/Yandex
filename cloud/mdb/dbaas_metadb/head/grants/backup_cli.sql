GRANT CONNECT ON DATABASE dbaas_metadb TO backup_cli;
GRANT USAGE ON SCHEMA dbaas TO backup_cli;

GRANT SELECT ON TABLE dbaas.clusters TO backup_cli;
GRANT SELECT ON TABLE dbaas.hosts TO backup_cli;
GRANT SELECT ON TABLE dbaas.shards TO backup_cli;
GRANT SELECT ON TABLE dbaas.subclusters TO backup_cli;
GRANT SELECT ON TABLE dbaas.backup_schedule TO backup_cli;
GRANT SELECT ON TABLE dbaas.pillar TO backup_cli;
GRANT SELECT, INSERT, UPDATE ON dbaas.backups TO backup_cli;
GRANT SELECT, INSERT, DELETE, UPDATE ON TABLE dbaas.backups_history TO backup_cli;
GRANT SELECT, INSERT, UPDATE ON TABLE dbaas.backups_import_history TO backup_cli;

/* Permissions for code.set_backup_service_use */

GRANT USAGE ON SCHEMA dbaas TO backup_cli;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO backup_cli;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO backup_cli;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA dbaas TO backup_cli;
GRANT USAGE ON SCHEMA code TO backup_cli;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO backup_cli;
