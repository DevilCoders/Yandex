ALTER TABLE mdb.containers
    ADD COLUMN pending_delete boolean NOT NULL DEFAULT false;
ALTER TABLE mdb.containers
    ADD COLUMN delete_token uuid;
ALTER TABLE mdb.volumes
    ADD COLUMN pending_backup boolean NOT NULL DEFAULT false;

COMMENT ON COLUMN mdb.containers.pending_delete
    IS 'If true container should be deleted';
COMMENT ON COLUMN mdb.volumes.pending_backup
    IS 'If true data would be copied before dropping container';
