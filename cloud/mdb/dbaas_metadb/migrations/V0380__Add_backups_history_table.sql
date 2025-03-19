CREATE TABLE dbaas.backups_history (
    backup_id   	 text NOT NULL,
    data_size        bigint NOT NULL,
    journal_size     bigint NOT NULL,
    committed_at     timestamptz DEFAULT now() NOT NULL,

    CONSTRAINT pk_backups_history PRIMARY KEY (backup_id, committed_at),
    CONSTRAINT fk_backups_history_backup_id FOREIGN KEY (backup_id) REFERENCES dbaas.backups(backup_id) ON DELETE CASCADE,
    CONSTRAINT check_sizes CHECK (data_size >= 0 AND journal_size >= 0)
);

GRANT SELECT ON TABLE dbaas.backups_history TO billing_bookkeeper;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_history TO backup_worker;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_history TO backup_cli;

GRANT SELECT ON TABLE dbaas.backups_history TO dbaas_support;
GRANT SELECT ON TABLE dbaas.backups_history TO katan_imp;
GRANT SELECT ON TABLE dbaas.backups_history TO logs_api;
GRANT SELECT ON TABLE dbaas.backups_history TO mdb_health;
GRANT SELECT ON TABLE dbaas.backups_history TO pillar_config;
GRANT SELECT ON TABLE dbaas.backups_history TO pillar_secrets;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_history TO cms;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_history TO dbaas_api;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_history TO dbaas_worker;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_history TO idm_service;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_history TO mdb_maintenance;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.backups_history TO mdb_ui;
