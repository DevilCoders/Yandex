ALTER TABLE mdb.volume_backups DROP CONSTRAINT pk_volume_backups;
ALTER TABLE mdb.volume_backups ADD CONSTRAINT pk_volume_backups PRIMARY KEY (container, path, dom0);
