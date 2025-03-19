CREATE TYPE mdb.volume_backend AS ENUM (
    'plain',
    'bind',
    'tmpfs',
    'quota',
    'native',
    'overlay',
    'loop',
    'rbd'
);

CREATE TABLE mdb.volumes (
    container       text NOT NULL,
    path            text NOT NULL DEFAULT '/',
    dom0            text NOT NULL,
    dom0_path       text NOT NULL,
    backend         mdb.volume_backend NOT NULL DEFAULT 'native',
    read_only       boolean NOT NULL DEFAULT false,

    /* In bytes */
    space_guarantee bigint,
    space_limit     bigint,

    inode_guarantee bigint,
    inode_limit     bigint,

    CONSTRAINT pk_volumes PRIMARY KEY (container, path),
    CONSTRAINT uk_volumes_dom0_path
            UNIQUE (dom0, dom0_path),
    CONSTRAINT fk_volumes_containers FOREIGN KEY (container)
        REFERENCES mdb.containers (fqdn) ON DELETE RESTRICT,
    CONSTRAINT fk_volumes_dom0_hosts FOREIGN KEY (dom0)
        REFERENCES mdb.dom0_hosts (fqdn) ON DELETE RESTRICT
);

COMMENT ON COLUMN mdb.volumes.space_guarantee IS 'In bytes';
COMMENT ON COLUMN mdb.volumes.space_limit IS 'In bytes';

INSERT INTO mdb.volumes (
        container, path, dom0, dom0_path, space_limit)
    (SELECT fqdn, '/' AS path, dom0, '/data/' || fqdn AS dom0_path,
            trim(trailing 'G' from space_limit)::bigint * 1024 * 1024 * 1024 AS space_limit
        FROM mdb.containers);
