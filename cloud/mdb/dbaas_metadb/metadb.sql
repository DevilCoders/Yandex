-- dbaas schema
-- This file should not be used in production!

CREATE SCHEMA dbaas;
CREATE EXTENSION IF NOT EXISTS pgcrypto;
CREATE EXTENSION IF NOT EXISTS plpython3u;


CREATE TABLE dbaas.clouds (
    cloud_id         bigserial NOT NULL,
    cloud_ext_id     text NOT NULL,
    cpu_quota        real NOT NULL,
    memory_quota     bigint NOT NULL,
    clusters_quota   bigint NOT NULL,
    cpu_used         real DEFAULT 0.0 NOT NULL,
    memory_used      bigint DEFAULT 0 NOT NULL,
    clusters_used    bigint DEFAULT 0 NOT NULL,
    actual_cloud_rev bigint NOT NULL,
    ssd_space_quota bigint NOT NULL,
    ssd_space_used bigint DEFAULT 0 NOT NULL,
    hdd_space_quota bigint NOT NULL,
    hdd_space_used bigint DEFAULT 0 NOT NULL,
    gpu_quota        bigint DEFAULT 0 NOT NULL,
    gpu_used         bigint DEFAULT 0 NOT NULL,

    CONSTRAINT pk_clouds PRIMARY KEY (cloud_id),

    CONSTRAINT check_quota CHECK (
        cpu_quota >= cpu_used
        AND gpu_quota >= gpu_used
        AND memory_quota >= memory_used
        AND ssd_space_quota >= ssd_space_used
        AND hdd_space_quota >= hdd_space_used
        AND clusters_quota >= clusters_used
    ),
    CONSTRAINT check_used_non_negative CHECK (
        cpu_used >= (0.0)
        AND gpu_used >= 0
        AND memory_used >= 0
        AND ssd_space_used >= 0
        AND hdd_space_used >= 0
        AND clusters_used >= 0
    ),
    CONSTRAINT check_cloud_ext_id CHECK (char_length(cloud_ext_id) <= 128)
);

CREATE UNIQUE INDEX uk_cloud_ext_id ON dbaas.clouds (cloud_ext_id);

CREATE TABLE dbaas.clouds_revs (
    cloud_id        bigint NOT NULL,
    cloud_rev       bigint NOT NULL,
    cpu_quota       real NOT NULL,
    memory_quota    bigint NOT NULL,
    clusters_quota  bigint NOT NULL,
    cpu_used        real  NOT NULL,
    memory_used     bigint NOT NULL,
    clusters_used   bigint NOT NULL,
    x_request_id    text,
    commited_at     timestamp with time zone DEFAULT now() NOT NULL,
    ssd_space_quota bigint NOT NULL,
    ssd_space_used  bigint NOT NULL,
    hdd_space_quota bigint NOT NULL,
    hdd_space_used  bigint NOT NULL,
    gpu_quota       bigint DEFAULT 0 NOT NULL,
    gpu_used        bigint DEFAULT 0 NOT NULL,

    CONSTRAINT pk_clouds_revs PRIMARY KEY (cloud_id, cloud_rev),
    CONSTRAINT fk_clouds_revs_cloud_id FOREIGN KEY (cloud_id)
        REFERENCES dbaas.clouds ON DELETE CASCADE
);

ALTER TABLE dbaas.clouds ADD CONSTRAINT fk_clouds_actual_cloud_rev
  FOREIGN KEY (cloud_id, actual_cloud_rev)
  REFERENCES dbaas.clouds_revs (cloud_id, cloud_rev)
  DEFERRABLE INITIALLY DEFERRED;


CREATE TABLE dbaas.folders (
    folder_id     bigserial NOT NULL,
    folder_ext_id text NOT NULL,
    cloud_id      bigint NOT NULL,

    CONSTRAINT pk_folders PRIMARY KEY (folder_id),
    CONSTRAINT fk_folders_cloud_id FOREIGN KEY (cloud_id)
        REFERENCES dbaas.clouds (cloud_id) ON DELETE RESTRICT,

    CONSTRAINT check_folder_ext_id CHECK (char_length(folder_ext_id) <= 128)
);

CREATE UNIQUE INDEX uk_folder_ext_id ON dbaas.folders (folder_ext_id);


CREATE TYPE dbaas.cluster_type AS ENUM (
    'postgresql_cluster',
    'pgaas-proxy',
    'zookeeper',
    'clickhouse_cluster',
    'mongodb_cluster',
    'redis_cluster',
    'mysql_cluster',
    'sqlserver_cluster',
    'greenplum_cluster',
    'hadoop_cluster',
    'kafka_cluster',
    'elasticsearch_cluster',
    'opensearch_cluster',
    'airflow_cluster',
    'metastore_cluster'
);

CREATE TYPE dbaas.env_type AS ENUM (
    'dev',
    'load',
    'qa',
    'prod',
    'compute-prod'
);

CREATE TYPE dbaas.action AS ENUM (
    'cluster-create',
    'cluster-delete',
    'cluster-stop',
    'cluster-start',
    'cluster-modify',
    'cluster-purge',
    'noop',
    'cluster-delete-metadata',
    'cluster-maintenance',
    'cluster-resetup',
    'cluster-offline-resetup'
);

CREATE TYPE dbaas.cluster_status AS ENUM (
    'CREATING',
    'CREATE-ERROR',
    'RUNNING',
    'MODIFYING',
    'MODIFY-ERROR',
    'STOPPING',
    'STOP-ERROR',
    'STOPPED',
    'STARTING',
    'START-ERROR',
    'DELETING',
    'DELETE-ERROR',
    'DELETED',
    'PURGING',
    'PURGE-ERROR',
    'PURGED',
    'METADATA-DELETING',
    'METADATA-DELETE-ERROR',
    'METADATA-DELETED',
    'RESTORING-ONLINE',
    'RESTORING-OFFLINE',
    'RESTORE-ONLINE-ERROR',
    'RESTORE-OFFLINE-ERROR',
    'MAINTAINING-OFFLINE',
    'MAINTAIN-OFFLINE-ERROR'
);

CREATE OR REPLACE FUNCTION dbaas.visible_cluster_status(
    dbaas.cluster_status
) RETURNS bool AS $$
SELECT $1 NOT IN (
    'DELETING',
    'DELETE-ERROR',
    'DELETED',
    'METADATA-DELETING',
    'METADATA-DELETE-ERROR',
    'METADATA-DELETED',
    'PURGING',
    'PURGE-ERROR',
    'PURGED'
);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION dbaas.active_cluster_status(
    dbaas.cluster_status
) RETURNS bool AS $$
SELECT $1 NOT IN (
    'RESTORING-OFFLINE',
    'RESTORE-OFFLINE-ERROR',
    'MAINTAINING-OFFLINE',
    'MAINTAIN-OFFLINE-ERROR',
    'STOPPING',
    'STOP-ERROR',
    'STOPPED',
    'DELETING',
    'DELETE-ERROR',
    'DELETED',
    'METADATA-DELETING',
    'METADATA-DELETE-ERROR',
    'METADATA-DELETED',
    'PURGING',
    'PURGE-ERROR',
    'PURGED'
);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION dbaas.error_cluster_status(
    dbaas.cluster_status
) RETURNS bool AS $$
SELECT $1 IN (
    'CREATE-ERROR',
    'MODIFY-ERROR',
    'STOP-ERROR',
    'START-ERROR',
    'DELETE-ERROR',
    'PURGE-ERROR',
    'METADATA-DELETE-ERROR',
    'RESTORE-ONLINE-ERROR',
    'RESTORE-OFFLINE-ERROR',
    'MAINTAIN-OFFLINE-ERROR'
);
$$ LANGUAGE SQL IMMUTABLE;

CREATE TABLE dbaas.clusters (
    cid                  text   NOT NULL,
    name                 text   NOT NULL,
    type                 dbaas.cluster_type NOT NULL,
    env                  dbaas.env_type NOT NULL,
    created_at           timestamp with time zone DEFAULT now() NOT NULL,
    public_key           bytea,
    network_id           text   NOT NULL,
    folder_id            bigint NOT NULL,
    description          text,
    status               dbaas.cluster_status DEFAULT 'CREATING'::dbaas.cluster_status NOT NULL,
    actual_rev           bigint NOT NULL,
    next_rev             bigint NOT NULL,
    host_group_ids       text[],
    deletion_protection  bool DEFAULT false NOT NULL,
    monitoring_cloud_id  text DEFAULT NULL,

    CONSTRAINT pk_clusters PRIMARY KEY (cid),
    CONSTRAINT fk_clusters_folder_id FOREIGN KEY (folder_id)
        REFERENCES dbaas.folders(folder_id) ON DELETE RESTRICT,

    CONSTRAINT check_cluster_cid CHECK (
        char_length(cid) >= 1
        AND char_length(cid) <= 256
    ),
    CONSTRAINT check_cluster_name CHECK (char_length(name) <= 256),
    CONSTRAINT check_description_length CHECK (char_length(description) < 512),
    CONSTRAINT check_next_rev_max CHECK (actual_rev <= next_rev)
);

CREATE TYPE dbaas.maintenance_task_status AS ENUM (
    'PLANNED',
    'CANCELED',
    'COMPLETED',
    'FAILED',
    'REJECTED'
);

CREATE TABLE dbaas.maintenance_tasks (
    cid         text NOT NULL,
    config_id   text NOT NULL,
    task_id     text,
    create_ts   timestamp with time zone DEFAULT now() NOT NULL,
    plan_ts     timestamp with time zone,
    status      dbaas.maintenance_task_status DEFAULT 'PLANNED'::dbaas.maintenance_task_status,
    info        text,
    max_delay   timestamp with time zone NOT NULL,

    CONSTRAINT pk_maintenance_tasks
        PRIMARY KEY (cid, config_id),
    CONSTRAINT fk_maintenance_tasks_cid FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE CASCADE
);
CREATE UNIQUE INDEX uk_maintenance_task_status_cid_where_planned
    ON dbaas.maintenance_tasks (cid)
WHERE status = 'PLANNED'::dbaas.maintenance_task_status;

-- we need that index for fk_clusters_folder_id
CREATE INDEX i_clusters_folder_id
    ON dbaas.clusters (folder_id);

CREATE UNIQUE INDEX uk_clusters_folder_id_type_name_for_visible
    ON dbaas.clusters (folder_id, type, name)
 WHERE dbaas.visible_cluster_status(status);

CREATE INDEX i_clusters_type_error_status
          ON dbaas.clusters (type)
       WHERE dbaas.error_cluster_status(status);

CREATE TABLE dbaas.clusters_revs (
    cid                 text NOT NULL,
    rev                 bigint NOT NULL,
    name                text NOT NULL,
    network_id          text NOT NULL,
    folder_id           bigint NOT NULL,
    description         text,
    status              dbaas.cluster_status NOT NULL,
    host_group_ids      text[],
    monitoring_cloud_id text DEFAULT NULL,

    CONSTRAINT pk_clusters_revs PRIMARY KEY (cid, rev),
    CONSTRAINT fk_clusters_revs_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters
);

ALTER TABLE dbaas.clusters
  ADD CONSTRAINT fk_clusters_cid_actual_rev_cluster_changes
  FOREIGN KEY (cid, actual_rev)
  REFERENCES dbaas.clusters_revs (cid, rev) DEFERRABLE INITIALLY DEFERRED;

CREATE TABLE dbaas.clusters_changes (
    cid          text     NOT NULL,
    rev          bigint   NOT NULL,
    changes      jsonb    NOT NULL DEFAULT '[]',
    committed_at timestamp with time zone DEFAULT now() NOT NULL,
    x_request_id text,
    commit_id    bigint   NOT NULL DEFAULT txid_current(),

    CONSTRAINT pk_clusters_changes PRIMARY KEY (cid, rev),
    CONSTRAINT fk_clusters_changes_clusters_revs FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs DEFERRABLE INITIALLY DEFERRED
);

CREATE INDEX i_clusters_changes_cid_committed_at
    ON dbaas.clusters_changes (cid, committed_at);

CREATE TYPE dbaas.role_type AS ENUM (
    'postgresql_cluster',
    'clickhouse_cluster',
    'pgaas-proxy',
    'mongodb_cluster',
    'zk',
    'redis_cluster',
    'mysql_cluster',
    'sqlserver_cluster',
    'windows_witness',
    'greenplum_cluster.segment_subcluster',
    'greenplum_cluster.master_subcluster',
    'mongodb_cluster.mongod',
    'mongodb_cluster.mongocfg',
    'mongodb_cluster.mongos',
    'mongodb_cluster.mongoinfra',
    'hadoop_cluster.masternode',
    'hadoop_cluster.datanode',
    'hadoop_cluster.computenode',
    'kafka_cluster',
    'elasticsearch_cluster.datanode',
    'elasticsearch_cluster.masternode',
    'opensearch_cluster.datanode',
    'opensearch_cluster.masternode',
    'airflow_cluster',
    'metastore_cluster'
);

CREATE TABLE dbaas.subclusters (
    subcid     text NOT NULL,
    cid        text NOT NULL,
    name       text NOT NULL,
    roles      dbaas.role_type[] NOT NULL,
    created_at timestamp with time zone DEFAULT now() NOT NULL,

    CONSTRAINT pk_subclusters PRIMARY KEY (subcid),
    CONSTRAINT fk_subclusters_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE RESTRICT,

    CONSTRAINT check_subcluster_name CHECK (char_length(name) <= 256),
    CONSTRAINT check_subcluster_subcid CHECK (
        char_length(subcid) >= 1
        AND char_length(subcid) <= 256
    )
);

CREATE INDEX i_subclusters_cid ON dbaas.subclusters (cid);
CREATE UNIQUE INDEX uk_subclusters_names ON dbaas.subclusters (cid, name);

CREATE TABLE dbaas.subclusters_revs (
    subcid     text NOT NULL,
    rev        bigint NOT NULL,
    cid        text NOT NULL,
    name       text NOT NULL,
    roles      dbaas.role_type[] NOT NULL,
    created_at timestamp with time zone NOT NULL,

    CONSTRAINT pk_subcluster_revs PRIMARY KEY (subcid, rev),
    CONSTRAINT fk_subclusters_revs_clusters_revs FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs
);

CREATE INDEX i_subclusters_revs_cid_rev
    ON dbaas.subclusters_revs (cid, rev);

CREATE TABLE dbaas.shards (
    subcid     text NOT NULL,
    shard_id   text NOT NULL,
    name       text NOT NULL,
    created_at timestamp with time zone DEFAULT now() NOT NULL,

    CONSTRAINT pk_shards PRIMARY KEY (shard_id),
    CONSTRAINT fk_shards_subclusters FOREIGN KEY (subcid)
        REFERENCES dbaas.subclusters(subcid) ON DELETE RESTRICT,
    CONSTRAINT check_shard_name CHECK (char_length(name) <= 256),
    CONSTRAINT check_shard_shard_id CHECK (
        char_length(shard_id) >= 1
        AND char_length(shard_id) <= 256
    )
);

CREATE INDEX i_shards_subcid ON dbaas.shards (subcid);
CREATE UNIQUE INDEX uk_shards_names ON dbaas.shards (subcid, name);

CREATE TABLE dbaas.shards_revs (
    subcid     text NOT NULL,
    shard_id   text NOT NULL,
    rev        bigint NOT NULL,
    name       text NOT NULL,
    created_at timestamp with time zone NOT NULL,

    CONSTRAINT pk_shards_revs PRIMARY KEY (shard_id, rev),
    CONSTRAINT fk_shards_revs_subclusters_rev FOREIGN KEY (subcid, rev)
        REFERENCES dbaas.subclusters_revs
);

CREATE INDEX i_shards_revs_subcid_rev
    ON dbaas.shards_revs (subcid, rev);

CREATE TABLE dbaas.regions (
    region_id      serial,
    name           text NOT NULL,
    cloud_provider text NOT NULL,
    timezone       text NOT NULL DEFAULT 'UTC',
    CONSTRAINT pk_region PRIMARY KEY (region_id)
);

CREATE UNIQUE INDEX uk_region_names ON dbaas.regions (name);

CREATE TABLE dbaas.geo (
    geo_id    serial,
    name      text NOT NULL,
    region_id integer,
    CONSTRAINT pk_geo PRIMARY KEY (geo_id),
    CONSTRAINT fk_geo_region FOREIGN KEY (region_id)
            REFERENCES dbaas.regions(region_id)
);

CREATE UNIQUE INDEX uk_geo_names ON dbaas.geo (name);

CREATE TYPE dbaas.space_quota_type AS ENUM (
    'ssd',
    'hdd'
);

CREATE TABLE dbaas.disk_type (
    disk_type_id                 serial NOT NULL,
    disk_type_ext_id             text NOT NULL,
    quota_type                   dbaas.space_quota_type NOT NULL,
    cloud_provider_disk_type     text DEFAULT NULL,
    -- should reflect https://cloud.yandex.ru/docs/compute/concepts/limits#compute-limits-disks
    allocation_unit_size         bigint,
    io_limit_per_allocation_unit bigint,
    io_limit_max                 bigint,

    CONSTRAINT pk_disk_type PRIMARY KEY (disk_type_id),
    CONSTRAINT check_disk_type_id CHECK (char_length(disk_type_ext_id) <= 128),
    CONSTRAINT check_limit_max CHECK (num_nulls(allocation_unit_size, io_limit_per_allocation_unit, io_limit_max) IN (0, 3))
);
COMMENT ON COLUMN dbaas.disk_type.cloud_provider_disk_type IS
    'The cloud providers name of disk type (disk type that we pass to their API)';
COMMENT ON CONSTRAINT check_limit_max ON dbaas.disk_type IS
    'All disk io_limit components should be defined or not defined at all';
CREATE UNIQUE INDEX uk_disk_types ON dbaas.disk_type (disk_type_ext_id);


CREATE TABLE dbaas.flavor_type (
    id          serial  NOT NULL,
    type        text    NOT NULL,
    generation  integer NOT NULL,

    CONSTRAINT pk_flavor_type PRIMARY KEY (type, generation),
    CONSTRAINT check_type CHECK (char_length(type) > 0 AND char_length(type) <= 32),
    CONSTRAINT check_generation CHECK (generation >= 0)
);

CREATE UNIQUE INDEX uk_fkavor_type ON dbaas.flavor_type (id);


CREATE TYPE dbaas.virt_type AS ENUM (
    'porto',
    'compute',
    'aws'
);

CREATE TABLE dbaas.flavors (
    id                         uuid DEFAULT public.gen_random_uuid() NOT NULL,
    cpu_guarantee              real DEFAULT 0.25 NOT NULL,
    cpu_limit                  real DEFAULT 1.00 NOT NULL,
    memory_guarantee           bigint DEFAULT 1073741824 NOT NULL,
    memory_limit               bigint DEFAULT 2147483648 NOT NULL,
    network_guarantee          bigint DEFAULT 1048576 NOT NULL,
    network_limit              bigint DEFAULT 16777216 NOT NULL,
    io_limit                   bigint DEFAULT 10485760 NOT NULL,
    name                       text NOT NULL,
    visible                    boolean DEFAULT true NOT NULL,
    vtype                      dbaas.virt_type DEFAULT 'porto'::dbaas.virt_type NOT NULL,
    platform_id                text NOT NULL,
    type                       text DEFAULT 'standard' NOT NULL,
    generation                 integer DEFAULT 1 NOT NULL,
    gpu_limit                  integer DEFAULT 0 NOT NULL,
    io_cores_limit             integer DEFAULT 0 NOT NULL,
    cloud_provider_flavor_name text DEFAULT NULL,
    arch                       text DEFAULT 'amd64' NOT NULL,

    CONSTRAINT pk_flavors PRIMARY KEY (id),
    CONSTRAINT fk_flavors_flavor_type FOREIGN KEY (type, generation)
        REFERENCES dbaas.flavor_type (type, generation) ON DELETE RESTRICT,
    CONSTRAINT check_flavor_name CHECK (char_length(name) <= 256)
);

COMMENT ON COLUMN dbaas.flavors.cpu_guarantee IS 'CPU cores';
COMMENT ON COLUMN dbaas.flavors.cpu_limit IS 'CPU cores';
COMMENT ON COLUMN dbaas.flavors.gpu_limit IS 'GPU cards';
COMMENT ON COLUMN dbaas.flavors.memory_guarantee IS 'bytes';
COMMENT ON COLUMN dbaas.flavors.memory_limit IS 'bytes';
COMMENT ON COLUMN dbaas.flavors.network_guarantee IS 'bytes per second';
COMMENT ON COLUMN dbaas.flavors.network_limit IS 'bytes per second';
COMMENT ON COLUMN dbaas.flavors.io_limit IS 'bytes per second';
COMMENT ON COLUMN dbaas.flavors.io_cores_limit IS 'Cores for software accelerated I/O';
COMMENT ON COLUMN dbaas.flavors.cloud_provider_flavor_name IS
    'The cloud providers name of flavor (flavor that we allocate)';
COMMENT ON COLUMN dbaas.flavors.arch IS
    'The architecture of flavor (flavor that we allocate)';
CREATE UNIQUE INDEX uk_flavors_name ON dbaas.flavors (name);


CREATE TABLE dbaas.hosts (
    subcid           text NOT NULL,
    shard_id         text,
    flavor           uuid NOT NULL,
    space_limit      bigint NOT NULL,
    fqdn             text NOT NULL,
    vtype_id         text,
    geo_id           integer NOT NULL,
    disk_type_id     integer NOT NULL,
    subnet_id        text NOT NULL,
    assign_public_ip boolean NOT NULL,
    created_at       timestamp with time zone DEFAULT now() NOT NULL,
    is_in_user_project_id  boolean DEFAULT false NOT NULL,

    CONSTRAINT pk_hosts PRIMARY KEY (fqdn),
    CONSTRAINT fk_hosts_disk_type_id FOREIGN KEY (disk_type_id)
        REFERENCES dbaas.disk_type(disk_type_id) ON DELETE RESTRICT,
    CONSTRAINT fk_hosts_flavor FOREIGN KEY (flavor)
        REFERENCES dbaas.flavors(id) ON DELETE RESTRICT,
    CONSTRAINT fk_hosts_geo FOREIGN KEY (geo_id)
        REFERENCES dbaas.geo(geo_id),
    CONSTRAINT fk_hosts_shards FOREIGN KEY (shard_id)
        REFERENCES dbaas.shards(shard_id) ON DELETE RESTRICT,
    CONSTRAINT fk_hosts_subclusters FOREIGN KEY (subcid)
        REFERENCES dbaas.subclusters(subcid) ON DELETE RESTRICT,


    CONSTRAINT check_host_name CHECK (char_length(fqdn) <= 256)
);

CREATE INDEX i_hosts_geo_id ON dbaas.hosts (geo_id);
CREATE INDEX i_hosts_shards ON dbaas.hosts (shard_id);
CREATE INDEX i_hosts_subcids ON dbaas.hosts (subcid);
CREATE INDEX i_hosts_vtype_id ON dbaas.hosts USING HASH (vtype_id);

CREATE TABLE dbaas.hosts_revs (
    subcid           text NOT NULL,
    shard_id         text,
    flavor           uuid NOT NULL,
    space_limit      bigint NOT NULL,
    fqdn             text NOT NULL,
    rev              bigint NOT NULL,
    vtype_id         text,
    geo_id           integer NOT NULL,
    disk_type_id     integer NOT NULL,
    subnet_id        text NOT NULL,
    assign_public_ip boolean NOT NULL,
    created_at       timestamp with time zone NOT NULL,
    is_in_user_project_id  boolean DEFAULT false NOT NULL,

    CONSTRAINT pk_hosts_revs PRIMARY KEY (fqdn, rev),
    CONSTRAINT fk_hosts_revs_disk_type_id FOREIGN KEY (disk_type_id)
        REFERENCES dbaas.disk_type(disk_type_id),
    CONSTRAINT fk_hosts_revs_flavor FOREIGN KEY (flavor)
        REFERENCES dbaas.flavors(id),
    CONSTRAINT fk_hosts_revs_geo FOREIGN KEY (geo_id)
        REFERENCES dbaas.geo(geo_id),
    CONSTRAINT fk_hosts_revs_shards_rev FOREIGN KEY (shard_id, rev)
        REFERENCES dbaas.shards_revs,
    CONSTRAINT fk_hosts_revs_subclusters_rev FOREIGN KEY (subcid, rev)
        REFERENCES dbaas.subclusters_revs
);

CREATE INDEX i_hosts_revs_geo_id
    ON dbaas.hosts_revs (geo_id);
CREATE INDEX i_hosts_revs_shards
    ON dbaas.hosts_revs (shard_id, rev);
CREATE INDEX i_hosts_revs_subcids
    ON dbaas.hosts_revs (subcid, rev);


CREATE TABLE dbaas.default_pillar (
    id    bigint NOT NULL DEFAULT 1,
    value jsonb NOT NULL DEFAULT '{}'::jsonb,

    CONSTRAINT pk_default_pillar PRIMARY KEY (id),
    CONSTRAINT check_default_pillar_id CHECK (id = 1)
);
INSERT INTO dbaas.default_pillar VALUES (1, '{}'::jsonb);


CREATE TABLE dbaas.cluster_type_pillar (
    type dbaas.cluster_type NOT NULL,
    value jsonb NOT NULL,

    CONSTRAINT pk_cluster_type_pillar PRIMARY KEY (type)
);


CREATE TABLE dbaas.role_pillar (
    type dbaas.cluster_type NOT NULL,
    role dbaas.role_type NOT NULL,
    value jsonb NOT NULL,

    CONSTRAINT pk_role_pillar PRIMARY KEY (type, role)
);


CREATE TABLE dbaas.pillar (
    cid      text,
    subcid   text,
    shard_id text,
    fqdn     text,
    value    jsonb NOT NULL,

    CONSTRAINT fk_pillar_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters (cid) ON DELETE RESTRICT,
    CONSTRAINT fk_pillar_subclusters FOREIGN KEY (subcid)
        REFERENCES dbaas.subclusters (subcid) ON DELETE RESTRICT,
    CONSTRAINT fk_pillar_shards FOREIGN KEY (shard_id)
        REFERENCES dbaas.shards (shard_id) ON DELETE RESTRICT,
    CONSTRAINT fk_pillar_hosts FOREIGN KEY (fqdn)
        REFERENCES dbaas.hosts (fqdn) ON DELETE RESTRICT,

    CONSTRAINT check_references CHECK (
        (cid IS NOT NULL AND subcid IS NULL AND
         shard_id IS NULL AND fqdn IS NULL)
        OR
        (cid IS NULL AND subcid IS NOT NULL AND
         shard_id IS NULL AND fqdn IS NULL)
        OR
        (cid IS NULL AND subcid IS NULL AND
         shard_id IS NOT NULL AND fqdn IS NULL)
        OR
        (cid IS NULL AND subcid IS NULL AND
         shard_id IS NULL AND fqdn IS NOT NULL)
    )
);

CREATE UNIQUE INDEX uk_pillar_cid ON dbaas.pillar (cid) WHERE (cid IS NOT NULL);
CREATE UNIQUE INDEX uk_pillar_fqdn ON dbaas.pillar (fqdn) WHERE (fqdn IS NOT NULL);
CREATE UNIQUE INDEX uk_pillar_shard ON dbaas.pillar (shard_id) WHERE (shard_id IS NOT NULL);
CREATE UNIQUE INDEX uk_pillar_subcid ON dbaas.pillar (subcid) WHERE (subcid IS NOT NULL);

CREATE INDEX i_pillar_value ON dbaas.pillar USING gin (value) WITH (fastupdate=off);

CREATE INDEX i_pillar_pgsync_cids ON dbaas.pillar (cid) WHERE cid IS NOT NULL AND value->'data'->'pgsync' IS NOT NULL;

CREATE TABLE dbaas.pillar_revs (
    rev      bigint NOT NULL,
    cid      text,
    subcid   text,
    shard_id text,
    fqdn     text,
    value    jsonb NOT NULL,

    CONSTRAINT fk_pillar_rev_clusters FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs,
    CONSTRAINT fk_pillar_rev_subclusters FOREIGN KEY (subcid, rev)
        REFERENCES dbaas.subclusters_revs,
    CONSTRAINT fk_pillar_rev_shards FOREIGN KEY (shard_id, rev)
        REFERENCES dbaas.shards_revs,
    CONSTRAINT fk_pillar_rev_hosts FOREIGN KEY (fqdn, rev)
        REFERENCES dbaas.hosts_revs
);

CREATE UNIQUE INDEX uk_pillar_revs_cid
    ON dbaas.pillar_revs (cid, rev) WHERE (cid IS NOT NULL);
CREATE UNIQUE INDEX uk_pillar_revs_fqdn
    ON dbaas.pillar_revs (fqdn, rev) WHERE (fqdn IS NOT NULL);
CREATE UNIQUE INDEX uk_pillar_revs_shard
    ON dbaas.pillar_revs (shard_id, rev) WHERE (shard_id IS NOT NULL);
CREATE UNIQUE INDEX uk_pillar_revs_subcid
    ON dbaas.pillar_revs (subcid, rev) WHERE (subcid IS NOT NULL);


CREATE TABLE dbaas.target_pillar (
    target_id text NOT NULL,
    cid       text,
    subcid    text,
    shard_id  text,
    fqdn      text,
    value     jsonb NOT NULL,

    CONSTRAINT fk_target_pillar_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE RESTRICT,
    CONSTRAINT fk_target_pillar_hosts FOREIGN KEY (fqdn)
        REFERENCES dbaas.hosts(fqdn),
    CONSTRAINT fk_target_pillar_shards FOREIGN KEY (shard_id)
        REFERENCES dbaas.shards(shard_id) ON DELETE RESTRICT,
    CONSTRAINT fk_target_pillar_subclusters FOREIGN KEY (subcid)
        REFERENCES dbaas.subclusters(subcid) ON DELETE RESTRICT,

    CONSTRAINT check_references CHECK (
        (cid IS NOT NULL AND subcid IS NULL AND
         shard_id IS NULL AND fqdn IS NULL)
        OR
        (cid IS NULL AND subcid IS NOT NULL AND
         shard_id IS NULL AND fqdn IS NULL)
        OR
        (cid IS NULL AND subcid IS NULL AND
         shard_id IS NOT NULL AND fqdn IS NULL)
        OR
        (cid IS NULL AND subcid IS NULL AND
         shard_id IS NULL AND fqdn IS NOT NULL)
    )
);

CREATE UNIQUE INDEX uk_target_pillar_cid
    ON dbaas.target_pillar (target_id, cid)
 WHERE (cid IS NOT NULL);
CREATE UNIQUE INDEX uk_target_pillar_fqdn
    ON dbaas.target_pillar (target_id, fqdn)
 WHERE (fqdn IS NOT NULL);
CREATE UNIQUE INDEX uk_target_pillar_shard
    ON dbaas.target_pillar (target_id, shard_id)
 WHERE (shard_id IS NOT NULL);
CREATE UNIQUE INDEX uk_target_pillar_subcid
    ON dbaas.target_pillar (target_id, subcid)
 WHERE (subcid IS NOT NULL);


CREATE TYPE dbaas.maintenance_window_days AS ENUM (
    'MON',
    'TUE',
    'WED',
    'THU',
    'FRI',
    'SAT',
    'SUN'
);

CREATE TABLE dbaas.maintenance_window_settings (
    cid     text NOT NULL,
    day     dbaas.maintenance_window_days NOT NULL,
    hour    integer NOT NULL,

    CONSTRAINT pk_maintenance_window_settings
        PRIMARY KEY (cid),
    CONSTRAINT fk_maintenance_window_settings_cid FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE CASCADE,

    CONSTRAINT maintenance_window_hour_min CHECK (hour >= 1),
    CONSTRAINT maintenance_window_hour_max CHECK (hour <= 24)
);

CREATE INDEX i_maintenance_window_settings_hour_and_day ON dbaas.maintenance_window_settings (day, hour);

CREATE TABLE dbaas.maintenance_window_settings_revs (
    cid         text NOT NULL,
    rev         bigint NOT NULL,
    day         dbaas.maintenance_window_days NOT NULL,
    hour        integer NOT NULL,
    CONSTRAINT pk_maintenance_window_settings_revs
        PRIMARY KEY (cid, rev),
    CONSTRAINT fk_maintenance_window_settings_revs_cid_rev FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs ON DELETE CASCADE
);

CREATE TABLE dbaas.worker_queue (
    task_id              text NOT NULL,
    cid                  text NOT NULL,
    create_ts            timestamp with time zone DEFAULT now() NOT NULL,
    start_ts             timestamp with time zone,
    end_ts               timestamp with time zone,
    worker_id            text,
    task_type            text NOT NULL,
    task_args            jsonb NOT NULL,
    result               boolean,
    changes              jsonb,
    comment              text,
    created_by           text,
    folder_id            bigint NOT NULL,
    operation_type       text NOT NULL,
    metadata             jsonb NOT NULL,
    hidden               boolean DEFAULT false NOT NULL,
    version              integer DEFAULT 1 NOT NULL,
    delayed_until        timestamp with time zone,
    required_task_id     text,
    errors               jsonb,
    context              jsonb,
    timeout              interval NOT NULL,
    create_rev           bigint NOT NULL,
    acquire_rev          bigint,
    finish_rev           bigint,
    unmanaged            boolean DEFAULT false NOT NULL,
    tracing              text,
    target_rev           bigint,
    config_id            text,
    restart_count        bigint DEFAULT 0 NOT NULL,
    failed_acquire_count bigint DEFAULT 0 NOT NULL,
    notes                text,
    cancelled            boolean,

    CONSTRAINT pk_worker_queue PRIMARY KEY (task_id),
    CONSTRAINT fk_worker_queue_cid FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_folder_id FOREIGN KEY (folder_id)
        REFERENCES dbaas.folders(folder_id) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_required_task_id FOREIGN KEY (required_task_id)
        REFERENCES dbaas.worker_queue(task_id) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_cid_config_id FOREIGN KEY (cid, config_id)
        REFERENCES dbaas.maintenance_tasks(cid, config_id),
    CONSTRAINT check_worker_queue_task_id CHECK (
        char_length(task_id) >= 1 AND char_length(task_id) <= 256
    ),
    CONSTRAINT worker_queue_task_id_not_equal_required_task_id CHECK (
        task_id != required_task_id
    ),
    CONSTRAINT worker_queue_not_failed_task_has_no_errors CHECK (
        ((result IS NULL OR result = true) AND errors IS NULL) OR (result = false)
    ),
    CONSTRAINT worker_queue_finished_tasks_has_no_context CHECK (
        ((result IS NOT NULL) AND context IS NULL) OR result IS NULL
    ),
    CONSTRAINT check_timeout_non_negative CHECK (
        extract(epoch FROM timeout) > 0
    ),
    CONSTRAINT check_maintenance_task_is_managed CHECK (
        (((target_rev IS NULL) AND (config_id IS NULL)) OR (NOT unmanaged))
    ),
    CONSTRAINT check_cancelled CHECK (
        (((result IS NULL) AND (cancelled) OR (NOT cancelled)))
    )
);
CREATE INDEX i_worker_queue_version_create_ts
    ON dbaas.worker_queue (version, create_ts)
 WHERE (start_ts IS NULL);
CREATE INDEX i_worker_queue_version_worker_id_task_id_running
    ON dbaas.worker_queue (version, worker_id, task_id)
 WHERE (end_ts IS NULL) AND (start_ts IS NOT NULL);
CREATE INDEX i_worker_queue_end_ts ON dbaas.worker_queue (cid, COALESCE(end_ts, 'infinity'::timestamp with time zone) DESC);
CREATE INDEX i_worker_queue_worker_id_failed ON dbaas.worker_queue (worker_id) WHERE (result = false);
CREATE UNIQUE INDEX uk_create_ts_tid ON dbaas.worker_queue (create_ts, task_id);
-- TODO: remove cid IS NULL from that index
CREATE UNIQUE INDEX uk_worker_active_tasks_managed
    ON dbaas.worker_queue (cid)
WHERE ((cid IS NOT NULL) AND (start_ts IS NOT NULL) AND (end_ts IS NULL)) AND (unmanaged = false);
CREATE INDEX i_worker_queue_require_task_id
    ON dbaas.worker_queue USING btree (required_task_id)
WHERE (required_task_id IS NOT NULL);
CREATE INDEX i_worker_queue_folder_id ON dbaas.worker_queue USING HASH (folder_id);
CREATE INDEX i_worker_queue_failed_acquire_count_task_id
    ON dbaas.worker_queue (failed_acquire_count, task_id)
 WHERE (failed_acquire_count > 0 AND result IS NULL);

-- it's not a problem that we don't add version to that index,
-- cause we shouldn't have lots of different pending-tasks-versions
CREATE INDEX i_worker_queue_pending_not_delayed
    ON dbaas.worker_queue (failed_acquire_count, create_ts)
 WHERE delayed_until IS NULL
   AND start_ts IS NULL
   AND unmanaged = false;

CREATE INDEX i_worker_queue_pending_and_delayed
    ON dbaas.worker_queue (failed_acquire_count, delayed_until)
 WHERE delayed_until IS NOT NULL
   AND start_ts IS NULL
   AND unmanaged = false;

CREATE UNIQUE INDEX uk_worker_queue_single_mw_config_task
    ON dbaas.worker_queue (cid, config_id)
    WHERE result IS NULL
    AND config_id IS NOT NULL;

CREATE TABLE dbaas.worker_queue_restart_history (
    task_id              text NOT NULL,
    cid                  text NOT NULL,
    create_ts            timestamp with time zone,
    start_ts             timestamp with time zone,
    end_ts               timestamp with time zone,
    worker_id            text,
    task_type            text NOT NULL,
    task_args            jsonb NOT NULL,
    result               boolean,
    changes              jsonb,
    comment              text,
    created_by           text,
    folder_id            bigint NOT NULL,
    operation_type       text NOT NULL,
    metadata             jsonb NOT NULL,
    hidden               boolean NOT NULL,
    version              integer NOT NULL,
    delayed_until        timestamp with time zone,
    required_task_id     text,
    errors               jsonb,
    context              jsonb,
    timeout              interval NOT NULL,
    create_rev           bigint NOT NULL,
    acquire_rev          bigint,
    finish_rev           bigint,
    unmanaged            boolean NOT NULL,
    tracing              text,
    target_rev           bigint,
    config_id            text,
    restart_count        bigint NOT NULL,
    failed_acquire_count bigint NOT NULL,
    notes                text,
    cancelled            boolean,

    CONSTRAINT pk_worker_queue_restart_history PRIMARY KEY (task_id, restart_count),
    CONSTRAINT fk_worker_queue_restart_history_cid FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_restart_history_folder_id FOREIGN KEY (folder_id)
        REFERENCES dbaas.folders(folder_id) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_restart_history_required_task_id FOREIGN KEY (required_task_id)
        REFERENCES dbaas.worker_queue(task_id) ON DELETE RESTRICT,
    CONSTRAINT fk_worker_queue_restart_history_cid_config_id FOREIGN KEY (cid, config_id)
        REFERENCES dbaas.maintenance_tasks(cid, config_id),
    CONSTRAINT check_worker_queue_restart_history_task_id CHECK (
        char_length(task_id) >= 1 AND char_length(task_id) <= 256
    )
);

-- feature flags
CREATE TABLE dbaas.default_feature_flags (
    flag_name text NOT NULL,

    CONSTRAINT pk_default_feature_flags PRIMARY KEY (flag_name),
    CONSTRAINT check_flag_name_len CHECK (char_length(flag_name) <= 256)
);


CREATE TABLE dbaas.cloud_feature_flags (
    cloud_id  bigint NOT NULL,
    flag_name text NOT NULL,
    CONSTRAINT pk_cloud_feature_flags PRIMARY KEY (cloud_id, flag_name),
    CONSTRAINT fk_cloud_feature_flags_cloud_id FOREIGN KEY (cloud_id)
        REFERENCES dbaas.clouds(cloud_id) ON DELETE RESTRICT,

    CONSTRAINT check_flag_name_len CHECK (char_length(flag_name) <= 256)
);


-- Labels
CREATE TABLE dbaas.cluster_labels (
    cid         text NOT NULL,
    label_key   text NOT NULL,
    label_value text NOT NULL,
    CONSTRAINT check_label_key_size CHECK (
        char_length(label_key) BETWEEN 1 AND 64
    ),
    CONSTRAINT check_label_value_size CHECK (
        char_length(label_value) BETWEEN 0 AND 64
    ),
    CONSTRAINT pk_cluster_labels PRIMARY KEY (cid, label_key),
    CONSTRAINT fk_cluster_labels_cid FOREIGN KEY (cid)
        REFERENCES dbaas.clusters ON DELETE CASCADE
);


CREATE TABLE dbaas.cluster_labels_revs (
    cid         text NOT NULL,
    label_key   text NOT NULL,
    rev         bigint NOT NULL,
    label_value text NOT NULL,
    CONSTRAINT pk_cluster_labels_revs PRIMARY KEY (cid, label_key, rev),
    CONSTRAINT fk_cluster_labels_revs_cid_rev FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs ON DELETE CASCADE
);


CREATE TYPE dbaas.config_host_type AS ENUM (
    'default',
    'pgaas-proxy',
    'dbaas-worker',
    'dataproc-api',
    'dataproc-ui-proxy'
);

CREATE TABLE dbaas.config_host_access_ids (
    access_id     uuid NOT NULL,
    access_secret text NOT NULL,
    active        boolean NOT NULL DEFAULT false,
    type          dbaas.config_host_type NOT NULL DEFAULT 'default'::dbaas.config_host_type,

    CONSTRAINT pk_config_host_access_ids PRIMARY KEY (access_id)
);


CREATE TABLE dbaas.idempotence (
    idempotence_id uuid NOT NULL,
    task_id        text NOT NULL,
    folder_id      bigint NOT NULL,
    user_id        text NOT NULL,
    request_hash   bytea NOT NULL,

    CONSTRAINT pk_idempotence PRIMARY KEY (idempotence_id, folder_id, user_id),
    CONSTRAINT fk_idempotence_folder_id FOREIGN KEY (folder_id)
        REFERENCES dbaas.folders(folder_id) ON DELETE RESTRICT,
    CONSTRAINT fk_idempotence_task_id FOREIGN KEY (task_id)
        REFERENCES dbaas.worker_queue(task_id) ON DELETE RESTRICT
);


CREATE TABLE dbaas.valid_resources (
    id                integer            NOT NULL,
    cluster_type      dbaas.cluster_type NOT NULL,
    role              dbaas.role_type    NOT NULL,
    flavor            uuid               NOT NULL,
    disk_type_id      integer            NOT NULL,
    geo_id            integer            NOT NULL,
    disk_size_range   int8range,
    disk_sizes        bigint[],
    min_hosts         integer            NOT NULL,
    max_hosts         integer            NOT NULL,
    feature_flag      text,
    default_disk_size bigint,

    CONSTRAINT pk_valid_resources PRIMARY KEY (id),
    CONSTRAINT fk_valid_resources_disk_type_id FOREIGN KEY (disk_type_id)
        REFERENCES dbaas.disk_type(disk_type_id) ON DELETE RESTRICT,
    CONSTRAINT fk_valid_resources_flavor FOREIGN KEY (flavor)
        REFERENCES dbaas.flavors(id) ON DELETE RESTRICT,
    CONSTRAINT fk_valid_resources_geo_id FOREIGN KEY (geo_id)
        REFERENCES dbaas.geo(geo_id) ON DELETE RESTRICT,
    CONSTRAINT check_disk_sizes CHECK (
        (disk_size_range IS NOT NULL AND disk_sizes IS NULL)
        OR (disk_sizes IS NOT NULL AND disk_size_range IS NULL)
    ),
    CONSTRAINT check_disk_sizes_cardinality CHECK (cardinality(disk_sizes) >= 1),
    CONSTRAINT check_disk_sizes_ndims CHECK (array_ndims(disk_sizes) = 1),
    CONSTRAINT check_min_hosts CHECK (min_hosts >= 0),
    CONSTRAINT check_max_hosts CHECK (max_hosts >= 0),
    CONSTRAINT check_hosts_count CHECK (max_hosts >= min_hosts),
    CONSTRAINT default_disk_size_valid CHECK (
            default_disk_size IS NULL OR (
                (disk_size_range IS NULL OR disk_size_range @> default_disk_size)
                AND (disk_sizes IS NULL OR default_disk_size = ANY (disk_sizes))
            )
        )
);

CREATE UNIQUE INDEX uk_valid_resources ON dbaas.valid_resources (cluster_type, role, flavor, disk_type_id, geo_id, coalesce(feature_flag, ''));

CREATE INDEX i_pillar_value_zk_id
    ON dbaas.pillar ((value->'data'->'pgsync'->'zk_id'))
    WHERE value->'data'->'pgsync' IS NOT NULL;


CREATE TABLE dbaas.search_queue (
    sq_id      bigint    GENERATED ALWAYS AS IDENTITY,
    created_at timestamp with time zone NOT NULL DEFAULT now(),
    queue_id   bigint,
    sent_at    timestamp with time zone,
    doc        jsonb     NOT NULL,

    CONSTRAINT pk_search_queue PRIMARY KEY (sq_id)
);

CREATE INDEX ip_created_at_not_queued
    ON dbaas.search_queue (created_at)
 WHERE queue_id IS NULL;

CREATE INDEX ip_queue_id_not_queued_and_not_sent
    ON dbaas.search_queue (queue_id)
    WHERE sent_at IS NULL AND queue_id IS NOT NULL;

CREATE SEQUENCE dbaas.search_queue_queue_ids AS bigint
    MINVALUE 1;

CREATE INDEX i_search_queue_queue_id
    ON dbaas.search_queue USING HASH (queue_id);

CREATE TABLE dbaas.worker_queue_events (
    event_id       bigint GENERATED ALWAYS AS IDENTITY,
    task_id        text   NOT NULL,
    data           jsonb  NOT NULL,
    start_sent_at  timestamp with time zone,
    done_sent_at   timestamp with time zone,

    CONSTRAINT pk_worker_queue_events PRIMARY KEY (event_id),
    CONSTRAINT fk_worker_queue_events FOREIGN KEY (task_id)
        REFERENCES dbaas.worker_queue(task_id) ON DELETE CASCADE,
    CONSTRAINT check_only_started_events_can_be_done CHECK (
        done_sent_at IS NULL OR (start_sent_at IS NOT NULL AND done_sent_at IS NOT NULL)
    )
);

CREATE UNIQUE INDEX uk_worker_queue_events_task_id
    ON dbaas.worker_queue_events (task_id);

CREATE UNIQUE INDEX ip_worker_queue_events_start_not_sent
    ON dbaas.worker_queue_events (event_id)
 WHERE start_sent_at IS NULL;

CREATE UNIQUE INDEX ip_worker_queue_events_done_not_sent
    ON dbaas.worker_queue_events (task_id)
 WHERE start_sent_at IS NOT NULL
   AND done_sent_at IS NULL;


CREATE SCHEMA IF NOT EXISTS stats;

CREATE OR REPLACE VIEW stats.quota AS
SELECT SUM(memory_quota) AS memory_quota,
       SUM(memory_used) AS memory_used,
       SUM(cpu_quota) AS cpu_quota,
       SUM(cpu_used) AS cpu_used,
       SUM(gpu_quota) AS gpu_quota,
       SUM(gpu_used) AS gpu_used,
       SUM(ssd_space_quota) AS ssd_space_quota,
       SUM(ssd_space_used) AS ssd_space_used,
       SUM(hdd_space_quota) AS hdd_space_quota,
       SUM(hdd_space_used) AS hdd_space_used,
       SUM(clusters_quota) AS clusters_quota,
       SUM(clusters_used) AS clusters_used
  FROM dbaas.clouds;

CREATE OR REPLACE VIEW stats.per_cluster_type AS
SELECT cluster_type,
       roles,
       flavors.type AS flavor_type,
       flavors.generation AS flavor_generation,
       flavors.name AS flavor_name,
       disk_type_ext_id AS disk_type,
       cpu_limit,
       memory_limit,
       network_limit,
       io_limit,
       hosts_count,
       ssd_space_used,
       hdd_space_used
  FROM (SELECT clusters.type AS cluster_type,
               roles,
               flavor AS flavor_id,
               disk_type_id,
               COUNT(*) AS hosts_count,
               SUM(space_limit) FILTER (WHERE quota_type = 'ssd'::dbaas.space_quota_type) AS ssd_space_used,
               SUM(space_limit) FILTER (WHERE quota_type = 'hdd'::dbaas.space_quota_type) AS hdd_space_used
          FROM dbaas.clusters
          JOIN dbaas.subclusters USING (cid)
          JOIN dbaas.hosts USING (subcid)
          JOIN dbaas.disk_type USING (disk_type_id)
         WHERE dbaas.visible_cluster_status(clusters.status)
         GROUP BY clusters.type,
                  roles,
                  flavor,
                  disk_type_id) s
  JOIN dbaas.flavors
    ON flavors.id = flavor_id
  JOIN dbaas.disk_type
 USING (disk_type_id);

CREATE TYPE dbaas.hadoop_jobs_status AS ENUM (
    'PROVISIONING',
    'PENDING',
    'CANCELLING',
    'RUNNING',
    'ERROR',
    'DONE',
    'CANCELLED'
);

CREATE TABLE dbaas.hadoop_jobs (
    job_id           text NOT NULL,
    cid              text NOT NULL,
    name             text NOT NULL,
    job_spec         jsonb NOT NULL,
    create_ts        timestamp with time zone DEFAULT now() NOT NULL,
    start_ts         timestamp with time zone,
    end_ts           timestamp with time zone,
    result           boolean,
    status           dbaas.hadoop_jobs_status DEFAULT 'PROVISIONING'::dbaas.hadoop_jobs_status NOT NULL,
    comment          text,
    created_by       text,
    application_info jsonb,

    CONSTRAINT pk_hadoop_jobs PRIMARY KEY (job_id),
    CONSTRAINT fk_hadoop_jobs_cid FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE RESTRICT,
    CONSTRAINT check_hadoop_job_id CHECK (
        char_length(job_id) >= 1 AND char_length(job_id) <= 256
    ),
    CONSTRAINT check_hadoop_job_start_ts CHECK (
        status < 'RUNNING' OR start_ts is NOT NULL
    ),
    CONSTRAINT check_hadoop_job_end_ts CHECK (
        status NOT IN ('ERROR', 'DONE', 'CANCELLED') OR end_ts IS NOT NULL
    )
);

CREATE INDEX ip_cluster_and_status ON dbaas.hadoop_jobs USING btree (cid, status, create_ts);
CREATE INDEX ip_hadoop_job_start_time ON dbaas.hadoop_jobs USING btree (create_ts);
CREATE INDEX ip_hadoop_job_end_time ON dbaas.hadoop_jobs USING btree (end_ts);

CREATE TABLE dbaas.instance_groups
(
    subcid            text NOT NULL,
    instance_group_id text,

    CONSTRAINT pk_instance_groups PRIMARY KEY (subcid),
    CONSTRAINT fk_instance_groups_subclusters FOREIGN KEY (subcid)
        REFERENCES dbaas.subclusters
);
CREATE INDEX i_instance_groups_instance_group_id ON dbaas.instance_groups (instance_group_id);

CREATE TABLE dbaas.instance_groups_revs
(
    subcid            text   NOT NULL,
    instance_group_id text,
    rev               bigint NOT NULL,

    CONSTRAINT pk_instance_groups_revs PRIMARY KEY (subcid, rev),
    CONSTRAINT fk_instance_groups_revs_subclusters_rev FOREIGN KEY (subcid, rev)
        REFERENCES dbaas.subclusters_revs
);

CREATE TABLE dbaas.backup_schedule(
    cid      text NOT NULL,
    schedule jsonb NOT NULL,

    CONSTRAINT pk_backup_schedule PRIMARY KEY (cid),
    CONSTRAINT fk_backup_schedule_cid FOREIGN KEY (cid)
       REFERENCES dbaas.clusters(cid) ON DELETE CASCADE
);

CREATE TABLE dbaas.backup_schedule_revs (
    cid          text NOT NULL,
    rev          bigint NOT NULL,
    schedule     jsonb NOT NULL,

    CONSTRAINT pk_backup_schedule_revs PRIMARY KEY (cid, rev),
    CONSTRAINT fk_backup_schedule_revs_clusters_revs FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs ON DELETE CASCADE
);

CREATE TYPE dbaas.backup_method AS ENUM (
    'FULL',
    'INCREMENTAL'
);

CREATE TYPE dbaas.backup_initiator AS ENUM (
    'SCHEDULE',
    'USER'
);

CREATE TYPE dbaas.backup_status AS ENUM (
	'PLANNED',
    'CREATING',
    'DONE',
    'OBSOLETE',
    'DELETING',
    'DELETED',
    'CREATE-ERROR',
    'DELETE-ERROR'
);

CREATE TABLE dbaas.backups (
    backup_id   	 text NOT NULL,
    cid 			 text NOT NULL,
    subcid		  	 text,
    shard_id    	 text,
    status      	 dbaas.backup_status NOT NULL,
    scheduled_date 	 date,
    created_at       timestamptz NOT NULL DEFAULT now(),
    delayed_until    timestamptz NOT NULL,
    started_at       timestamptz,
    finished_at      timestamptz,
    updated_at       timestamptz,
    shipment_id  	 text,
	metadata    	 jsonb,
    errors       	 jsonb,
    initiator        dbaas.backup_initiator NOT NULL,
    method           dbaas.backup_method NOT NULL,

    CONSTRAINT pk_backups PRIMARY KEY (backup_id),
    CONSTRAINT fk_backups_cid FOREIGN KEY (cid) REFERENCES dbaas.clusters(cid) ON DELETE CASCADE,
    CONSTRAINT check_backups_initiator_scheduled_date CHECK (
        (initiator = 'SCHEDULE' AND scheduled_date IS NOT NULL)
        OR
        (initiator = 'USER' AND scheduled_date IS NULL)
    )
);

CREATE UNIQUE INDEX uk_backups_cid_subcid_shard_id_scheduled_date ON dbaas.backups (cid, subcid, shard_id, scheduled_date) WHERE scheduled_date IS NOT NULL;
CREATE INDEX i_finished_at_status ON dbaas.backups (finished_at, status);
CREATE INDEX i_delayed_until_status ON dbaas.backups (delayed_until, status);
CREATE INDEX i_status_finished_at ON dbaas.backups (status, finished_at);
CREATE INDEX i_status_delayed_until ON dbaas.backups (status, delayed_until);

CREATE TABLE dbaas.backups_dependencies (
    parent_id    text REFERENCES dbaas.backups(backup_id) ON DELETE RESTRICT,
    child_id     text REFERENCES dbaas.backups(backup_id) ON DELETE CASCADE,

    CONSTRAINT pk_backups_dependencies PRIMARY KEY (parent_id, child_id)
);

CREATE INDEX i_backups_dependencies_child_id ON dbaas.backups_dependencies (child_id);

CREATE TYPE dbaas.sg_type AS ENUM (
    'service',
    'user'
);

CREATE TABLE dbaas.sgroups (
    cid          text NOT NULL,
    sg_ext_id    text NOT NULL,
    sg_type      dbaas.sg_type NOT NULL,
    sg_hash      integer DEFAULT 0 NOT NULL,
    sg_allow_all boolean default false not null,

    CONSTRAINT pk_sgroups PRIMARY KEY (cid, sg_ext_id),
    CONSTRAINT fk_sgroups_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters(cid) ON DELETE CASCADE,

    CONSTRAINT check_sg_ext_id CHECK (
        char_length(sg_ext_id) >= 1
        AND char_length(sg_ext_id) <= 256
    )
);

CREATE TABLE dbaas.sgroups_revs (
    cid          text NOT NULL,
    sg_ext_id    text NOT NULL,
    sg_type      dbaas.sg_type NOT NULL,
    rev          bigint NOT NULL,
    sg_hash      integer DEFAULT 0 NOT NULL,
    sg_allow_all boolean default false not null,

    CONSTRAINT pk_sgroups_revs PRIMARY KEY (cid, rev, sg_ext_id),
    CONSTRAINT fk_sgroups_revs_clusters_revs FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs ON DELETE CASCADE
);

CREATE TABLE dbaas.versions (
    cid             text,
    subcid          text,
    shard_id        text,
    component       text NOT NULL,
    major_version   text NOT NULL,
    minor_version   text NOT NULL,
    package_version text NOT NULL,
    edition         text DEFAULT 'default'::text NOT NULL,
    pinned          BOOLEAN DEFAULT FALSE,

    CONSTRAINT fk_versions_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters (cid) ON DELETE RESTRICT,
    CONSTRAINT fk_versions_subclusters FOREIGN KEY (subcid)
        REFERENCES dbaas.subclusters (subcid) ON DELETE RESTRICT,
    CONSTRAINT fk_versions_shards FOREIGN KEY (shard_id)
        REFERENCES dbaas.shards (shard_id) ON DELETE RESTRICT,

    CONSTRAINT check_references CHECK (num_nonnulls(cid, subcid, shard_id) = 1)
);

CREATE UNIQUE INDEX uk_versions_cid_component ON dbaas.versions (cid, component) WHERE (cid IS NOT NULL);
CREATE UNIQUE INDEX uk_versions_subcid_component ON dbaas.versions (subcid, component) WHERE (subcid IS NOT NULL);
CREATE UNIQUE INDEX uk_versions_shard_component ON dbaas.versions (shard_id, component) WHERE (shard_id IS NOT NULL);

CREATE TABLE dbaas.versions_revs (
    cid             text,
    subcid          text,
    shard_id        text,
    component       text NOT NULL,
    major_version   text NOT NULL,
    minor_version   text NOT NULL,
    package_version text NOT NULL,
    rev             bigint NOT NULL,
    edition         text DEFAULT 'default'::text NOT NULL,
    pinned          BOOLEAN DEFAULT FALSE,

    CONSTRAINT fk_versions_revs_clusters_rev FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs (cid, rev) ON DELETE RESTRICT,
    CONSTRAINT fk_versions_revs_subclusters_rev FOREIGN KEY (subcid, rev)
        REFERENCES dbaas.subclusters_revs (subcid, rev) ON DELETE RESTRICT,
    CONSTRAINT fk_versions_revs_shards_rev FOREIGN KEY (shard_id, rev)
        REFERENCES dbaas.shards_revs (shard_id, rev) ON DELETE RESTRICT,

    CONSTRAINT check_references CHECK (num_nonnulls(cid, subcid, shard_id) = 1)
);

CREATE UNIQUE INDEX uk_versions_revs_cid_component_rev ON dbaas.versions_revs (cid, component, rev) WHERE (cid IS NOT NULL);
CREATE UNIQUE INDEX uk_versions_revs_subcid_component_rev ON dbaas.versions_revs (subcid, component, rev) WHERE (subcid IS NOT NULL);
CREATE UNIQUE INDEX uk_versions_revs_shard_component_rev ON dbaas.versions_revs (shard_id, component, rev) WHERE (shard_id IS NOT NULL);

CREATE TABLE dbaas.default_versions (
    type            dbaas.cluster_type NOT NULL,
    component       text NOT NULL,
    major_version   text NOT NULL,
    minor_version   text NOT NULL,
    package_version text NOT NULL,
    env             dbaas.env_type NOT NULL,
    is_deprecated   boolean NOT NULL,
    is_default      boolean NOT NULL,
    updatable_to    text[],
    name            text NOT NULL,
    edition         text DEFAULT 'default'::text NOT NULL,

    CONSTRAINT pk_default_versions PRIMARY KEY (type, component, name, env)
);

CREATE UNIQUE INDEX uk_id_default_versions ON dbaas.default_versions (type, component, env) WHERE is_default;

CREATE OR REPLACE FUNCTION dbaas.default_versions_check_updatable_to() RETURNS TRIGGER
    LANGUAGE plpgsql AS
$function$
DECLARE
    v_row    dbaas.default_versions;
    _name    text;
BEGIN
    v_row = NEW;

    RAISE NOTICE 'Checking updatable_to % of default_version (type %, component %, name %, env %)',
        v_row.updatable_to, v_row.type, v_row.component, v_row.name, v_row.env;
    IF v_row.updatable_to IS NULL THEN
        RETURN NULL;
    END IF;
    FOREACH _name IN ARRAY v_row.updatable_to
        LOOP
            IF NOT EXISTS(
                    SELECT 1
                    FROM dbaas.default_versions
                    WHERE type = v_row.type
                      AND component = v_row.component
                      AND name = _name
                      AND env = v_row.env
                ) THEN
                RAISE EXCEPTION 'Incorrect name % in updatable_to of default_version (type %, component %, name %, env %)',
                    _name, v_row.type, v_row.component, v_row.name, v_row.env;
            END IF;
        END LOOP;

    -- RETURN value of AFTER trigger is always ignored
    RETURN NULL;
END;
$function$;

CREATE CONSTRAINT TRIGGER tg_default_versions_check_updatable_to
    AFTER INSERT OR UPDATE OF updatable_to
    ON dbaas.default_versions
    DEFERRABLE INITIALLY DEFERRED
    FOR EACH ROW
EXECUTE PROCEDURE dbaas.default_versions_check_updatable_to();

CREATE OR REPLACE FUNCTION dbaas.default_versions_check_name() RETURNS TRIGGER
    LANGUAGE plpgsql AS
$function$
DECLARE
    v_row         dbaas.default_versions;
    _version      dbaas.default_versions;
BEGIN
    v_row = OLD;

    RAISE NOTICE 'Checking old name % of default_version (type %, component %, name %, env %)',
        v_row.name, v_row.type, v_row.component, v_row.name, v_row.env;
    SELECT *
    INTO _version
    FROM dbaas.default_versions
    WHERE type = v_row.type
      AND component = v_row.component
      AND env = v_row.env
      AND updatable_to IS NOT NULL
      AND v_row.name = ANY (updatable_to::text[]);

    IF FOUND AND NOT EXISTS(
            SELECT 1
            FROM dbaas.default_versions
            WHERE type = v_row.type
              AND component = v_row.component
              AND env = v_row.env
              AND name = v_row.name
        ) THEN
        RAISE EXCEPTION 'Old name of default_version (type %, component %, name %, env %) contained in updatable_to (type %, component %, name %, env %)',
            v_row.type, v_row.component, v_row.name, v_row.env,
            _version.type, _version.component, _version.name, _version.env;
    END IF;

    -- RETURN value of AFTER trigger is always ignored
    RETURN NULL;
END;
$function$;

CREATE CONSTRAINT TRIGGER tg_default_versions_check_name
    AFTER DELETE OR UPDATE OF name
    ON dbaas.default_versions
    DEFERRABLE INITIALLY DEFERRED
    FOR EACH ROW
EXECUTE PROCEDURE dbaas.default_versions_check_name();

ALTER TABLE dbaas.pillar ADD CONSTRAINT check_host_cert_expiration_valid_timestamp_with_tz CHECK (
     value->>'cert.expiration'
          ~ '^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]{1,6})?(Z|[+-][0-9]{2}:[0-9]{2})$'
);

COMMENT ON CONSTRAINT check_host_cert_expiration_valid_timestamp_with_tz ON dbaas.pillar
    IS 'check that cert.expiration contains timestamp with specified time zone';

CREATE OR REPLACE FUNCTION dbaas.string_to_iso_timestamptz(ts text) RETURNS timestamptz
    LANGUAGE SQL
    IMMUTABLE STRICT AS
$function$
SELECT ts::timestamptz;
$function$;

CREATE INDEX i_pillar_expiration_fqdn
    ON dbaas.pillar ((dbaas.string_to_iso_timestamptz(value ->> 'cert.expiration')), fqdn)
    WHERE fqdn IS NOT NULL;

CREATE TYPE dbaas.alert_condition AS ENUM (
    'LESS',
    'LESS_OR_EQUAL',
    'EQUAL',
    'NOT_EQUAL',
    'GREATER_OR_EQUAL',
    'GREATER'
);

CREATE TYPE dbaas.alert_group_status AS ENUM (
    'CREATING',
    'ACTIVE',
    'DELETING',
    'CREATE-ERROR',
    'DELETE-ERROR'
);

CREATE TYPE dbaas.alert_status AS ENUM (
    'CREATING',
    'ACTIVE',
    'UPDATING',
    'DELETING',
    'CREATE-ERROR',
    'DELETE-ERROR'
);

CREATE OR REPLACE FUNCTION dbaas.visible_alert_status(
    dbaas.alert_status
) RETURNS bool AS $$
SELECT $1 IN (
	'ACTIVE',
	'CREATING',
	'UPDATING',
	'CREATE-ERROR'
);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION dbaas.visible_alert_group_status(
    dbaas.alert_group_status
) RETURNS bool AS $$
SELECT $1 IN (
	'ACTIVE',
	'CREATING',
	'CREATE-ERROR'
);
$$ LANGUAGE SQL IMMUTABLE;

CREATE TABLE dbaas.alert_group (
    alert_group_id        TEXT NOT NULL,
    cid                   TEXT NOT NULL,
    monitoring_folder_id  TEXT NOT NULL,
    managed               BOOLEAN NOT NULL DEFAULT FALSE,
    status                dbaas.alert_group_status NOT NULL,

    CONSTRAINT pk_alert_group PRIMARY KEY (alert_group_id),
    CONSTRAINT fk_alert_group_cid FOREIGN KEY (cid) REFERENCES dbaas.clusters(cid) ON DELETE RESTRICT
);

CREATE UNIQUE INDEX uk_alert_group_cid_id ON dbaas.alert_group (cid, alert_group_id);

CREATE TABLE dbaas.alert_group_revs (
    alert_group_id        TEXT NOT NULL,
    rev                   BIGINT NOT NULL,
    cid                   TEXT NOT NULL,
    monitoring_folder_id  TEXT NOT NULL,
    managed               BOOLEAN NOT NULL DEFAULT FALSE,
    status                dbaas.alert_group_status NOT NULL,

    CONSTRAINT pk_alert_group_revs PRIMARY KEY (alert_group_id, rev),
    CONSTRAINT fk_alert_group_revs_cid FOREIGN KEY (cid, rev) REFERENCES dbaas.clusters_revs(cid, rev) ON DELETE RESTRICT
);

CREATE UNIQUE INDEX uk_alert_group_revs_cid_id ON dbaas.alert_group_revs (cid, rev, alert_group_id);

CREATE TABLE dbaas.default_alert (
    critical_threshold NUMERIC,
    warning_threshold  NUMERIC,
    cluster_type       dbaas.cluster_type NOT NULL,
    template_id        TEXT NOT NULL,
    template_version   TEXT NOT NULL,
    description        TEXT,
    name               TEXT,
    mandatory          BOOLEAN DEFAULT FALSE,

    CONSTRAINT pk_default_alert PRIMARY KEY (template_id)
);

CREATE TABLE dbaas.alert (
    alert_ext_id          TEXT,
    alert_group_id        TEXT NOT NULL,
    template_id           TEXT NOT NULL,
    notification_channels TEXT[] NOT NULL CHECK(notification_channels <> '{}'),
    disabled              BOOLEAN NOT NULL DEFAULT FALSE,
    critical_threshold    NUMERIC,
    warning_threshold     NUMERIC,
    default_thresholds    BOOLEAN NOT NULL,
    status                dbaas.alert_status NOT NULL,

    CONSTRAINT pk_alert_group_mertic_name PRIMARY KEY (alert_group_id, template_id),
    CONSTRAINT alert_has_thresholds CHECK (critical_threshold IS NOT NULL or warning_threshold IS NOT NULL),
    CONSTRAINT fk_alert_alert_group FOREIGN KEY (alert_group_id) REFERENCES dbaas.alert_group(alert_group_id) ON DELETE RESTRICT,
    CONSTRAINT fk_alert_default_alert FOREIGN KEY (template_id) REFERENCES dbaas.default_alert(template_id) ON DELETE RESTRICT
);

CREATE TABLE dbaas.alert_revs (
    alert_ext_id          TEXT,
    alert_group_id        TEXT NOT NULL,
    template_id           TEXT NOT NULL,
    rev                   BIGINT NOT NULL,
    notification_channels TEXT[] NOT NULL CHECK(notification_channels <> '{}'),
    disabled              BOOLEAN NOT NULL DEFAULT FALSE,
    critical_threshold    NUMERIC,
    warning_threshold     NUMERIC,
    default_thresholds    BOOLEAN NOT NULL,
    status                dbaas.alert_status NOT NULL,

    CONSTRAINT pk_alert_group_mertic_name_rev PRIMARY KEY (alert_group_id, template_id, rev),
    CONSTRAINT fk_alert_alert_group_rev FOREIGN KEY (alert_group_id, rev) REFERENCES dbaas.alert_group_revs(alert_group_id, rev) ON DELETE RESTRICT
);

CREATE TYPE dbaas.disk_placement_group_status AS ENUM (
    'DESCRIBED',
    'CREATING',
    'CREATE-ERROR',
    'COMPLETE',
    'DELETING',
    'DELETE-ERROR',
    'DELETED'
);

CREATE TABLE dbaas.disk_placement_groups (
    pg_id                       bigint GENERATED BY DEFAULT AS IDENTITY,
    cid                         text NOT NULL,
    local_id                    bigint NOT NULL,
    disk_placement_group_id     text,
    status                      dbaas.disk_placement_group_status NOT NULL,

    CONSTRAINT pk_disk_placement_groups PRIMARY KEY (pg_id),

    CONSTRAINT fk_disk_placement_groups_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters (cid) ON DELETE RESTRICT
);

CREATE UNIQUE INDEX uk_disk_placement_groups_cid_local_id ON dbaas.disk_placement_groups (cid, local_id);
CREATE UNIQUE INDEX uk_disk_placement_groups_disk_placement_group_id ON dbaas.disk_placement_groups (disk_placement_group_id) WHERE (disk_placement_group_id IS NOT NULL);

comment on column dbaas.disk_placement_groups.disk_placement_group_id is 'The placement group id from API';
comment on column dbaas.disk_placement_groups.local_id is 'local ID of placement group in cluster';

CREATE TABLE dbaas.disk_placement_groups_revs (
    rev                         bigint NOT NULL,
    pg_id                       bigint NOT NULL,
    cid                         text NOT NULL,
    local_id                    bigint NOT NULL,
    disk_placement_group_id     text,
    status                      dbaas.disk_placement_group_status NOT NULL,

    CONSTRAINT pk_disk_placement_groups_revs PRIMARY KEY (pg_id, rev),

    CONSTRAINT fk_disk_placement_groups_revs_clusters_revs FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs (cid, rev) ON DELETE CASCADE
);

CREATE INDEX ix_disk_placement_groups_revs_cid_rev
    ON dbaas.disk_placement_groups_revs(cid, rev);


CREATE TYPE dbaas.disk_status AS ENUM (
    'DESCRIBED',
    'CREATING',
    'CREATE-ERROR',
    'COMPLETE',
    'DELETING',
    'DELETE-ERROR',
    'DELETED'
);

CREATE TABLE dbaas.disks (
    d_id                        bigint GENERATED BY DEFAULT AS IDENTITY,
    pg_id                       bigint NOT NULL,
    fqdn                        text NOT NULL,
    mount_point                 text NOT NULL,
    disk_id                     text,
    host_disk_id                text,
    status                      dbaas.disk_status NOT NULL,
    cid                         text NOT NULL,

    CONSTRAINT pk_disks PRIMARY KEY (d_id),

    CONSTRAINT fk_disks_disk_placement_groups FOREIGN KEY (pg_id)
        REFERENCES dbaas.disk_placement_groups (pg_id) ON DELETE RESTRICT,
    CONSTRAINT fk_disks_hosts FOREIGN KEY (fqdn)
        REFERENCES dbaas.hosts (fqdn) ON DELETE RESTRICT,
    CONSTRAINT fk_disks_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters (cid) ON DELETE RESTRICT
);

CREATE UNIQUE INDEX uk_disks_fqdn_mount_point ON dbaas.disks (fqdn, mount_point);


CREATE INDEX i_disks_cid
    ON dbaas.disks(cid);

comment on column dbaas.disks.disk_id is 'The disk id from API';
comment on column dbaas.disks.host_disk_id is 'Host device name like `/dev/vdb1`';

CREATE TABLE dbaas.disks_revs (
    rev                         bigint NOT NULL,
    d_id                        bigint NOT NULL,
    pg_id                       bigint NOT NULL,
    fqdn                        text NOT NULL,
    mount_point                 text NOT NULL,
    disk_id                     text,
    host_disk_id                text,
    status                      dbaas.disk_status NOT NULL,
    cid                         text NOT NULL,

    CONSTRAINT pk_disks_revs PRIMARY KEY (d_id, rev),

    CONSTRAINT disks_revs_clusters_revs FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs (cid, rev) ON DELETE CASCADE,
    CONSTRAINT disks_revs_hosts_revs FOREIGN KEY (fqdn, rev)
        REFERENCES dbaas.hosts_revs (fqdn, rev) ON DELETE CASCADE
);

CREATE INDEX i_disks_revs_cid_rev
    ON dbaas.disks_revs(cid, rev);

CREATE TYPE dbaas.placement_group_status AS ENUM (
    'DESCRIBED',
    'CREATING',
    'CREATE-ERROR',
    'COMPLETE',
    'DELETING',
    'DELETE-ERROR',
    'DELETED'
);

CREATE TABLE dbaas.placement_groups (
    pg_id                       bigint GENERATED BY DEFAULT AS IDENTITY,
    cid                         text NOT NULL,
    subcid                      text,
    shard_id                    text,
    placement_group_id          text,
    status                      dbaas.placement_group_status NOT NULL,
    fqdn                        text,
    local_id                    bigint,

    CONSTRAINT pk_placement_groups PRIMARY KEY (pg_id),
    CONSTRAINT fk_placement_groups_clusters FOREIGN KEY (cid)
        REFERENCES dbaas.clusters (cid) ON DELETE RESTRICT,
    CONSTRAINT fk_placement_groups_hostname FOREIGN KEY (fqdn)
        REFERENCES dbaas.hosts (fqdn) ON DELETE CASCADE
);

CREATE UNIQUE INDEX uk_placement_groups_fqdn ON dbaas.placement_groups (fqdn);
CREATE INDEX i_placement_groups_cid ON dbaas.placement_groups (cid);
CREATE INDEX i_placement_groups_placement_group_id ON dbaas.placement_groups (placement_group_id) WHERE (placement_group_id IS NOT NULL);

comment on column dbaas.placement_groups.placement_group_id is 'The placement group id from API';

CREATE TABLE dbaas.placement_groups_revs (
    pg_id                       bigint GENERATED BY DEFAULT AS IDENTITY,
    rev                         bigint NOT NULL,
    cid                         text NOT NULL,
    subcid                      text,
    shard_id                    text,
    placement_group_id          text,
    status                      dbaas.placement_group_status NOT NULL,
    fqdn                        text,
    local_id                    bigint,

    CONSTRAINT pk_placement_groups_revs PRIMARY KEY (pg_id, rev),
    CONSTRAINT fk_placement_groups_revs_clusters_revs FOREIGN KEY (cid, rev)
        REFERENCES dbaas.clusters_revs (cid, rev) ON DELETE CASCADE
);

CREATE INDEX i_placement_groups_revs_cid_rev
    ON dbaas.placement_groups_revs(cid, rev);


CREATE TABLE dbaas.kubernetes_node_groups
(
    subcid                   text NOT NULL,
    kubernetes_cluster_id    text,
    node_group_id text,

    CONSTRAINT pk_kubernetes_node_groups PRIMARY KEY (subcid),
    CONSTRAINT fk_kubernetes_node_groups_subclusters FOREIGN KEY (subcid)
        REFERENCES dbaas.subclusters
);
CREATE INDEX i_kubernetes_node_groups_kubernetes_cluster_id ON dbaas.kubernetes_node_groups (kubernetes_cluster_id);
CREATE INDEX i_kubernetes_node_groups_node_group_id ON dbaas.kubernetes_node_groups (node_group_id);

CREATE TABLE dbaas.kubernetes_node_groups_revs
(
    subcid                text   NOT NULL,
    kubernetes_cluster_id text,
    node_group_id         text,
    rev                   bigint NOT NULL,

    CONSTRAINT pk_kubernetes_node_groups_revs PRIMARY KEY (subcid, rev),
    CONSTRAINT fk_kubernetes_node_groups_revs_subclusters_rev FOREIGN KEY (subcid, rev)
        REFERENCES dbaas.subclusters_revs
);

ALTER TABLE dbaas.worker_queue
    ADD CONSTRAINT check_worker_task_created_by_not_null CHECK (created_by IS NOT NULL);

ALTER TABLE dbaas.worker_queue
    ALTER COLUMN created_by SET DEFAULT '';

CREATE TABLE dbaas.backups_history (
    backup_id   	 text NOT NULL,
    data_size        bigint NOT NULL,
    journal_size     bigint NOT NULL,
    committed_at     timestamptz DEFAULT now() NOT NULL,

    CONSTRAINT pk_backups_history PRIMARY KEY (backup_id, committed_at),
    CONSTRAINT fk_backups_history_backup_id FOREIGN KEY (backup_id) REFERENCES dbaas.backups(backup_id) ON DELETE CASCADE,
    CONSTRAINT check_sizes CHECK (data_size >= 0 AND journal_size >= 0)
);

CREATE TABLE dbaas.backups_import_history (
    cid 			text NOT NULL,
    last_import_at  timestamptz NOT NULL,
    errors       	jsonb,

    CONSTRAINT fk_backups_import_history_cid FOREIGN KEY (cid) REFERENCES dbaas.clusters(cid) ON DELETE CASCADE
);

CREATE UNIQUE INDEX uk_backups_import_history_cid
    ON dbaas.backups_import_history (cid)
    INCLUDE (last_import_at);
