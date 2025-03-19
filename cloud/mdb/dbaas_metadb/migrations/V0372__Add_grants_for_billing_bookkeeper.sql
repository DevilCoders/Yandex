--head/grants/billing_bookkeeper.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO billing_bookkeeper;
GRANT USAGE ON SCHEMA dbaas TO billing_bookkeeper;
GRANT USAGE ON SCHEMA code TO billing_bookkeeper;

GRANT SELECT ON TABLE dbaas.clusters TO billing_bookkeeper;
GRANT SELECT ON TABLE dbaas.shards TO billing_bookkeeper;
GRANT SELECT ON TABLE dbaas.subclusters TO billing_bookkeeper;
GRANT SELECT ON TABLE dbaas.hosts TO billing_bookkeeper;
GRANT SELECT ON TABLE dbaas.hosts_revs TO billing_bookkeeper;
GRANT SELECT ON TABLE dbaas.backup_schedule TO billing_bookkeeper;
GRANT SELECT ON TABLE dbaas.backups TO billing_bookkeeper;
GRANT SELECT ON TABLE dbaas.backups_dependencies TO billing_bookkeeper;
GRANT SELECT ON TABLE dbaas.clusters_changes TO billing_bookkeeper;
