GRANT CONNECT ON DATABASE dbaas_metadb TO backup_worker;
GRANT USAGE ON SCHEMA dbaas TO backup_worker;

GRANT SELECT ON TABLE dbaas.clusters TO backup_worker;
GRANT SELECT ON TABLE dbaas.hosts TO backup_worker;
GRANT SELECT ON TABLE dbaas.shards TO backup_worker;
GRANT SELECT ON TABLE dbaas.subclusters TO backup_worker;
GRANT SELECT ON TABLE dbaas.backup_schedule TO backup_worker;
GRANT SELECT ON TABLE dbaas.pillar TO backup_worker;
GRANT SELECT, INSERT, UPDATE ON dbaas.backups TO backup_worker;
GRANT SELECT ON TABLE dbaas.versions TO backup_worker;
GRANT SELECT ON TABLE dbaas.versions_revs TO backup_worker;
GRANT SELECT ON dbaas.backups_dependencies TO backup_worker;
GRANT SELECT ON TABLE dbaas.clusters_changes TO backup_worker;
GRANT SELECT, INSERT, DELETE, UPDATE ON TABLE dbaas.backups_history TO backup_worker;
