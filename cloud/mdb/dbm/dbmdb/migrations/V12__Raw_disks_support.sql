CREATE TABLE mdb.disks (
    id              uuid NOT NULL,
    max_space_limit bigint NOT NULL,
    has_data        boolean NOT NULL,
    dom0            text NOT NULL,

    CONSTRAINT pk_disks PRIMARY KEY (id),
    CONSTRAINT fk_disks_dom0_hosts FOREIGN KEY (dom0)
        REFERENCES mdb.dom0_hosts (fqdn) ON DELETE RESTRICT,
    CONSTRAINT check_max_space_limit CHECK (max_space_limit > 0)
);

CREATE UNIQUE INDEX uk_disks_dom0_id ON mdb.disks (dom0, id);

ALTER TABLE mdb.volumes ADD COLUMN disk_id uuid;

CREATE INDEX i_volumes_disk_id ON mdb.volumes (disk_id);

ALTER TABLE mdb.volumes ADD CONSTRAINT
    fk_volumes_disks FOREIGN KEY (disk_id, dom0)
    REFERENCES mdb.disks (id, dom0) ON DELETE RESTRICT;

ALTER TABLE mdb.volumes ADD CONSTRAINT
    check_volumes_disk_path CHECK
    (disk_id IS NULL OR position(('/disks/' || disk_id::text || '/') in dom0_path) = 1);
