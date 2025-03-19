CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE mdb.volume_backups (
    container      text NOT NULL,
    path           text NOT NULL,
    dom0           text NOT NULL,
    dom0_path      text NOT NULL,
    delete_token   uuid NOT NULL DEFAULT public.gen_random_uuid(),
    pending_delete boolean NOT NULL DEFAULT false,
    create_ts      timestamp with time zone NOT NULL DEFAULT now(),
    space_limit    bigint,
    disk_id        uuid,

    CONSTRAINT pk_volume_backups PRIMARY KEY (container, path),
    CONSTRAINT uk_volume_backups_dom0_path UNIQUE (dom0, dom0_path),
    CONSTRAINT fk_volume_backups_dom0_hosts FOREIGN KEY (dom0)
        REFERENCES mdb.dom0_hosts (fqdn) ON DELETE RESTRICT
);

COMMENT ON COLUMN mdb.volume_backups.space_limit IS 'In bytes';

CREATE INDEX i_volume_backups_create_ts ON mdb.volume_backups (create_ts);
