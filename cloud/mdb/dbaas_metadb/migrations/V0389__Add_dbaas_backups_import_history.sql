CREATE TABLE dbaas.backups_import_history (
    cid 			text NOT NULL,
    last_import_at  timestamptz NOT NULL,
    errors       	jsonb,

    CONSTRAINT fk_backups_import_history_cid FOREIGN KEY (cid) REFERENCES dbaas.clusters(cid) ON DELETE CASCADE
);

CREATE UNIQUE INDEX uk_backups_import_history_cid
    ON dbaas.backups_import_history (cid)
    INCLUDE (last_import_at);

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_import_history TO backup_cli;

GRANT SELECT ON TABLE dbaas.backups_import_history TO dbaas_support;
GRANT SELECT ON TABLE dbaas.backups_import_history TO katan_imp;
GRANT SELECT ON TABLE dbaas.backups_import_history TO logs_api;
GRANT SELECT ON TABLE dbaas.backups_import_history TO mdb_health;
GRANT SELECT ON TABLE dbaas.backups_import_history TO pillar_config;
GRANT SELECT ON TABLE dbaas.backups_import_history TO pillar_secrets;

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_import_history TO cms;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_import_history TO dbaas_api;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_import_history TO dbaas_worker;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_import_history TO idm_service;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_import_history TO mdb_maintenance;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_import_history TO mdb_ui;

