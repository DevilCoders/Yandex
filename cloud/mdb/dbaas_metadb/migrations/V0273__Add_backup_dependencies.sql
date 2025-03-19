CREATE TABLE dbaas.backups_dependencies (
    parent_id    text REFERENCES dbaas.backups(backup_id) ON DELETE RESTRICT,
    child_id     text REFERENCES dbaas.backups(backup_id) ON DELETE CASCADE,
    CONSTRAINT pk_backups_dependencies PRIMARY KEY (parent_id, child_id)
);
CREATE INDEX i_backups_dependencies_child_id ON dbaas.backups_dependencies (child_id);
