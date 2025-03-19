ALTER TABLE mdb.containers DROP COLUMN space_limit;
ALTER TABLE mdb.containers DROP COLUMN storage_class;
DROP TYPE mdb.storage_class;

ALTER TABLE mdb.dom0_hosts ALTER COLUMN memory_gb TYPE bigint
    USING memory_gb::bigint * 1024 * 1024 * 1024;
ALTER TABLE mdb.dom0_hosts RENAME COLUMN memory_gb TO memory;

ALTER TABLE mdb.dom0_hosts ALTER COLUMN ssd_space_gb TYPE bigint
    USING ssd_space_gb::bigint * 1024 * 1024 * 1024;
ALTER TABLE mdb.dom0_hosts RENAME COLUMN ssd_space_gb TO ssd_space;

ALTER TABLE mdb.dom0_hosts ALTER COLUMN sata_space_gb TYPE bigint
    USING sata_space_gb::bigint * 1024 * 1024 * 1024;
ALTER TABLE mdb.dom0_hosts RENAME COLUMN sata_space_gb TO sata_space;

ALTER TABLE mdb.dom0_hosts ALTER COLUMN max_io_mbps TYPE bigint
    USING max_io_mbps::bigint * 1024 * 1024;
ALTER TABLE mdb.dom0_hosts RENAME COLUMN max_io_mbps TO max_io;

ALTER TABLE mdb.dom0_hosts ALTER COLUMN net_speed_gbps TYPE bigint
    USING net_speed_gbps::bigint * 1024 * 1024 * 1024 / 8;
ALTER TABLE mdb.dom0_hosts RENAME COLUMN net_speed_gbps TO net_speed;

COMMENT ON COLUMN mdb.dom0_hosts.memory IS 'In bytes';
COMMENT ON COLUMN mdb.dom0_hosts.ssd_space IS 'In bytes';
COMMENT ON COLUMN mdb.dom0_hosts.sata_space IS 'In bytes';

COMMENT ON COLUMN mdb.dom0_hosts.max_io IS 'In bytes per second';
COMMENT ON COLUMN mdb.dom0_hosts.net_speed IS 'In bytes per second';
