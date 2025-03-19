GRANT CONNECT ON DATABASE dbaas_metadb TO backup_scheduler;
GRANT USAGE ON SCHEMA dbaas TO backup_scheduler;
GRANT USAGE ON SCHEMA code TO backup_scheduler;

GRANT SELECT ON TABLE dbaas.clusters TO backup_scheduler;
GRANT SELECT ON TABLE dbaas.shards TO backup_scheduler;
GRANT SELECT ON TABLE dbaas.subclusters TO backup_scheduler;
GRANT SELECT ON TABLE dbaas.backup_schedule TO backup_scheduler;
GRANT SELECT, INSERT, UPDATE, DELETE ON dbaas.backups TO backup_scheduler;
GRANT SELECT, INSERT, UPDATE, DELETE ON dbaas.backups_dependencies TO backup_scheduler;
GRANT SELECT ON TABLE dbaas.versions_revs TO backup_scheduler;
GRANT SELECT ON TABLE dbaas.clusters_changes TO backup_scheduler;
