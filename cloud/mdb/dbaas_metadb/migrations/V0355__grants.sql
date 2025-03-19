--head/grants/backup_cli.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO backup_cli;
GRANT USAGE ON SCHEMA dbaas TO backup_cli;

GRANT SELECT ON TABLE dbaas.clusters TO backup_cli;
GRANT SELECT ON TABLE dbaas.hosts TO backup_cli;
GRANT SELECT ON TABLE dbaas.shards TO backup_cli;
GRANT SELECT ON TABLE dbaas.subclusters TO backup_cli;
GRANT SELECT ON TABLE dbaas.backup_schedule TO backup_cli;
GRANT SELECT ON TABLE dbaas.pillar TO backup_cli;
GRANT SELECT, INSERT, UPDATE ON dbaas.backups TO backup_cli;


/* Permissions for code.set_backup_service_use */

GRANT USAGE ON SCHEMA dbaas TO backup_cli;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO backup_cli;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO backup_cli;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA dbaas TO backup_cli;
GRANT USAGE ON SCHEMA code TO backup_cli;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO backup_cli;

--head/grants/backup_scheduler.sql
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

--head/grants/backup_worker.sql
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

--head/grants/cloud_dwh.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO cloud_dwh;
GRANT USAGE ON SCHEMA dbaas TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.backups TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.backups_dependencies TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.backup_schedule TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.cloud_feature_flags TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.clouds TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.clusters TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.clusters_changes TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.clusters_revs TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.disk_type TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.disks TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.flavor_type TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.flavors TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.folders TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.geo TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.hosts TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.instance_groups TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.maintenance_tasks TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.maintenance_window_settings TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.sgroups TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.shards TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.subclusters TO cloud_dwh;
GRANT SELECT ON TABLE dbaas.versions TO cloud_dwh;

--head/grants/cms.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO cms;
GRANT USAGE ON SCHEMA dbaas TO cms;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO cms;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO cms;
GRANT USAGE ON SCHEMA code TO cms;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO cms;

--head/grants/dataproc_health.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO dataproc_health;
GRANT USAGE ON SCHEMA dbaas TO dataproc_health;
GRANT SELECT ON TABLE dbaas.clusters TO dataproc_health;

--head/grants/dbaas_api.sql
GRANT USAGE ON SCHEMA dbaas TO dbaas_api;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO dbaas_api;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO dbaas_api;
GRANT USAGE ON SCHEMA code TO dbaas_api;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO dbaas_api;

--head/grants/dbaas_support.sql
GRANT USAGE ON SCHEMA dbaas TO dbaas_support;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO dbaas_support;

GRANT INSERT, UPDATE, DELETE ON dbaas.clouds TO dbaas_support;
GRANT INSERT, UPDATE, DELETE ON dbaas.folders TO dbaas_support;
GRANT UPDATE ON dbaas.clusters TO dbaas_support;

GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO dbaas_support;
GRANT USAGE ON SCHEMA code TO dbaas_support;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO dbaas_support;

--head/grants/dbaas_worker.sql
GRANT USAGE ON SCHEMA dbaas TO dbaas_worker;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO dbaas_worker;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO dbaas_worker;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA dbaas TO dbaas_worker;
GRANT USAGE ON SCHEMA code TO dbaas_worker;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO dbaas_worker;

--head/grants/idm_service.sql
GRANT USAGE ON SCHEMA dbaas TO idm_service;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO idm_service;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO idm_service;
GRANT USAGE ON SCHEMA code TO idm_service;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO idm_service;

--head/grants/katan_imp.sql
GRANT USAGE ON SCHEMA dbaas TO katan_imp;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO katan_imp;

GRANT USAGE ON SCHEMA code TO katan_imp;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO katan_imp;

--head/grants/logs_api.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO logs_api;
GRANT USAGE ON SCHEMA dbaas TO logs_api;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO logs_api;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO logs_api;
GRANT USAGE ON SCHEMA code TO logs_api;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO logs_api;

--head/grants/mdb_dns.sql
GRANT USAGE ON SCHEMA dbaas TO mdb_dns;
GRANT USAGE ON SCHEMA code TO mdb_dns;
GRANT SELECT ON TABLE dbaas.clusters TO mdb_dns;
GRANT SELECT ON TABLE dbaas.shards TO mdb_dns;
GRANT SELECT ON TABLE dbaas.subclusters TO mdb_dns;
GRANT SELECT ON TABLE dbaas.worker_queue TO mdb_dns;

--head/grants/mdb_downtimer.sql
GRANT USAGE ON SCHEMA dbaas TO mdb_downtimer;

GRANT SELECT ON TABLE dbaas.clusters TO mdb_downtimer;
GRANT SELECT ON TABLE dbaas.folders TO mdb_downtimer;
GRANT SELECT ON TABLE dbaas.subclusters TO mdb_downtimer;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA dbaas TO mdb_downtimer;

GRANT USAGE ON SCHEMA code TO mdb_downtimer;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO mdb_downtimer;

--head/grants/mdb_event_producer.sql
GRANT USAGE ON SCHEMA dbaas TO mdb_event_producer;

GRANT SELECT, INSERT, UPDATE ON TABLE dbaas.worker_queue_events TO mdb_event_producer;
GRANT SELECT ON TABLE dbaas.worker_queue TO mdb_event_producer;


--head/grants/mdb_health.sql
GRANT USAGE ON SCHEMA dbaas TO mdb_health;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO mdb_health;

GRANT USAGE ON SCHEMA code TO mdb_health;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO mdb_health;

--head/grants/mdb_maintenance.sql
GRANT USAGE ON SCHEMA dbaas TO mdb_maintenance;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO mdb_maintenance;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO mdb_maintenance;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA dbaas TO mdb_maintenance;
GRANT USAGE ON SCHEMA code TO mdb_maintenance;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO mdb_maintenance;

GRANT CONNECT ON DATABASE dbaas_metadb TO mdb_maintenance;

--head/grants/mdb_report.sql
GRANT USAGE ON SCHEMA dbaas TO mdb_report;
GRANT SELECT ON TABLE dbaas.clusters TO mdb_report;
GRANT SELECT ON TABLE dbaas.subclusters TO mdb_report;
GRANT SELECT ON TABLE dbaas.hosts TO mdb_report;
GRANT SELECT ON TABLE dbaas.pillar TO mdb_report;
GRANT SELECT ON TABLE dbaas.flavors TO mdb_report;
GRANT SELECT ON TABLE dbaas.disk_type TO mdb_report;
GRANT SELECT ON TABLE dbaas.folders TO mdb_report;
GRANT SELECT ON TABLE dbaas.clouds TO mdb_report;
GRANT SELECT ON TABLE dbaas.worker_queue TO mdb_report;
GRANT USAGE ON SCHEMA code TO mdb_report;
GRANT EXECUTE ON FUNCTION code.managed(dbaas.clusters) TO mdb_report;

--head/grants/mdb_search_producer.sql
GRANT USAGE ON SCHEMA dbaas TO mdb_search_producer;

GRANT SELECT, INSERT, UPDATE ON TABLE dbaas.search_queue TO mdb_search_producer;
GRANT ALL ON SEQUENCE dbaas.search_queue_queue_ids TO mdb_search_producer;

--head/grants/mdb_ui.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO mdb_ui;
GRANT USAGE ON SCHEMA dbaas TO mdb_ui;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA dbaas TO mdb_ui;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO mdb_ui;
GRANT USAGE ON SCHEMA code TO mdb_ui;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO mdb_ui;

--head/grants/metadb_admin.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO metadb_admin;

--head/grants/monitor.sql
GRANT USAGE ON SCHEMA dbaas TO monitor;
GRANT SELECT ON TABLE dbaas.worker_queue TO monitor;

--head/grants/pillar_config.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO pillar_config;
GRANT USAGE ON SCHEMA dbaas TO pillar_config;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO pillar_config;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO pillar_config;
GRANT USAGE ON SCHEMA code TO pillar_config;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO pillar_config;

--head/grants/pillar_secrets.sql
GRANT CONNECT ON DATABASE dbaas_metadb TO pillar_secrets;
GRANT USAGE ON SCHEMA dbaas TO pillar_secrets;
GRANT SELECT ON ALL TABLES IN SCHEMA dbaas TO pillar_secrets;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA dbaas TO pillar_secrets;
GRANT USAGE ON SCHEMA code TO pillar_secrets;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA code TO pillar_secrets;
