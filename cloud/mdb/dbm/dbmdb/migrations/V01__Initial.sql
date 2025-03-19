CREATE SCHEMA IF NOT EXISTS mdb;

CREATE TABLE mdb.locations (
    geo     text NOT NULL,

    CONSTRAINT pk_locations PRIMARY KEY (geo)
);

CREATE TABLE mdb.projects (
    name        text NOT NULL,
    description text,

    CONSTRAINT pk_projects PRIMARY KEY (name)
);

CREATE TABLE mdb.dom0_hosts (
    fqdn            text NOT NULL,
    project         text NOT NULL,
    geo             text NOT NULL,

    cpu_cores       integer NOT NULL,
    memory_gb       integer NOT NULL,
    ssd_space_gb    integer NOT NULL,
    sata_space_gb   integer NOT NULL DEFAULT 0,
    max_io_mbps     integer NOT NULL,
    net_speed_gbps  integer NOT NULL,

    allow_new_hosts  boolean NOT NULL DEFAULT true,

    CONSTRAINT pk_dom0_hosts PRIMARY KEY (fqdn),
    CONSTRAINT fk_dom0_hosts_projects_name FOREIGN KEY (project)
        REFERENCES mdb.projects (name) ON DELETE RESTRICT,
    CONSTRAINT fk_dom0_hosts_locations_geo FOREIGN KEY (geo)
        REFERENCES mdb.locations ON DELETE RESTRICT
);

COMMENT ON COLUMN mdb.dom0_hosts.max_io_mbps IS 'In megabytes per second';
COMMENT ON COLUMN mdb.dom0_hosts.net_speed_gbps IS 'In gigabits per second';

CREATE TABLE mdb.clusters (
    name    text NOT NULL,
    project text NOT NULL,

    CONSTRAINT pk_clusters PRIMARY KEY (name),
    CONSTRAINT fk_clusters_projects_name FOREIGN KEY (project)
        REFERENCES mdb.projects ON DELETE RESTRICT
);

CREATE TYPE mdb.storage_class AS ENUM (
    'ssd',
    'sata'
);

CREATE TABLE mdb.containers (
    dom0                text NOT NULL,
    fqdn                text NOT NULL,
    cluster_name        text NOT NULL,

    /* In cores */
    cpu_guarantee       integer,
    cpu_limit           integer,
    /* In bytes */
    memory_guarantee    bigint,
    memory_limit        bigint,
    hugetlb_limit       bigint,
    /* In bytes per second */
    net_guarantee       bigint,
    net_limit           bigint,
    io_limit            bigint,

    /* I.e. 100G */
    space_limit         text,
    storage_class       mdb.storage_class DEFAULT 'ssd',

    extra_properties    jsonb,

    CONSTRAINT pk_containers PRIMARY KEY (fqdn),
    CONSTRAINT fk_containers_cluster_name FOREIGN KEY (cluster_name)
        REFERENCES mdb.clusters (name) ON DELETE RESTRICT,
    CONSTRAINT fk_containers_dom0_hosts FOREIGN KEY (dom0)
        REFERENCES mdb.dom0_hosts (fqdn) ON DELETE RESTRICT
);

COMMENT ON COLUMN mdb.containers.cpu_guarantee IS 'In cores';
COMMENT ON COLUMN mdb.containers.cpu_limit IS 'In cores';
COMMENT ON COLUMN mdb.containers.memory_guarantee IS 'In bytes';
COMMENT ON COLUMN mdb.containers.memory_limit IS 'In bytes';
COMMENT ON COLUMN mdb.containers.hugetlb_limit IS 'In bytes';
COMMENT ON COLUMN mdb.containers.net_guarantee IS 'In bytes per second';
COMMENT ON COLUMN mdb.containers.net_limit IS 'In bytes per second';
COMMENT ON COLUMN mdb.containers.io_limit IS 'In bytes per second';
COMMENT ON COLUMN mdb.containers.space_limit IS 'I.e. 100G';
