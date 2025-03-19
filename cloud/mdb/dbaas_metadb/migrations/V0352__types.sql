
--head/code/00_schema.sql
CREATE SCHEMA IF NOT EXISTS code;

-- Sadly PostgreSQL hasn't CREATE TYPE IF NOT EXISTS syntax, so wrap CREATE TYPE in the query that checks type existence.

DO
$$
BEGIN
IF NOT EXISTS (
    SELECT 1
      FROM pg_type typ
      JOIN pg_namespace nsp
        ON nsp.oid = typ.typnamespace
     WHERE nsp.nspname = 'code'
       AND typ.typname = 'pillar_priority')
THEN
    --head/code/05_types.sql
    CREATE TYPE code.pillar_priority AS ENUM (
        'default',
        'cluster_type',
        'role',
        'cid',      'target_cid',
        'subcid',   'target_subcid',
        'shard_id', 'target_shard_id',
        'fqdn',     'target_fqdn'
    );

    CREATE TYPE code.pillar_with_priority AS (
        value    jsonb,
        priority code.pillar_priority
    );

    CREATE TYPE code.version_priority AS ENUM (
        'cid',
        'subcid',
        'shard_id'
    );

    CREATE TYPE code.version AS (
        component       text,
        major_version   text,
        minor_version   text,
        package_version text,
        edition         text
    );

    CREATE TYPE code.host AS (
        subcid           text,
        shard_id         text,
        space_limit      bigint,
        flavor_id        uuid,
        subnet_id        text,
        geo              text,
        disk_type_id     text,
        fqdn             text,
        vtype            dbaas.virt_type,
        vtype_id         text,
        roles            dbaas.role_type[],
        subcluster_name  text,
        assign_public_ip boolean,
        environment      dbaas.env_type,
        flavor_name      text,
        created_at       timestamp with time zone,
        host_group_ids   text[],
        cluster_type     dbaas.cluster_type
    );

    CREATE TYPE code.operation_status AS ENUM (
        'PENDING',
        'RUNNING',
        'FAILED',
        'DONE'
    );

    CREATE TYPE code.operation AS (
        operation_id          text,
        target_id             text,
        cid                   text,
        cluster_type          dbaas.cluster_type,
        env                   dbaas.env_type,
        operation_type        text,
        created_by            text,
        created_at            timestamp with time zone,
        started_at            timestamp with time zone,
        modified_at           timestamp with time zone,
        status                code.operation_status,
        metadata              jsonb,
        args                  jsonb,  -- remove it later
        hidden                boolean,
        required_operation_id text,
        errors                jsonb
    );

    CREATE TYPE code.idempotence_data AS (
        idempotence_id uuid,
        request_hash   bytea
    );

    CREATE TYPE code.paging AS (
        last_date timestamp with time zone,
        last_id   text
    );

    CREATE TYPE code.operation_id_by_idempotence AS (
        operation_id text,
        request_hash bytea
    );

    CREATE TYPE code.label AS (
        key   text,
        value text
    );

    CREATE TYPE code.cluster_with_labels AS (
        cid                     text,
        rev                     bigint,
        next_rev                bigint,
        name                    text,
        type                    dbaas.cluster_type,
        folder_id               bigint,
        env                     dbaas.env_type,
        created_at              timestamp with time zone,
        status                  dbaas.cluster_status,
        pillar_value            jsonb,
        network_id              text,
        description             text,
        labels                  code.label[],
        mw_day                  dbaas.maintenance_window_days,
        mw_hour                 int,
        mw_config_id            text,
        mw_max_delay            timestamp with time zone,
        mw_delayed_until        timestamp with time zone,
        mw_create_ts            timestamp with time zone,
        mw_info                 text,
        maintenance_window      jsonb,
        backup_schedule         jsonb,
        user_sgroup_ids         text[],
        host_group_ids          text[],
        deletion_protection     bool
    );

    CREATE DOMAIN code.worker_id AS text
        CONSTRAINT worker_id_is_not_null_and_not_empty
            CHECK (VALUE IS NOT NULL AND length(VALUE) > 0);

    CREATE DOMAIN code.task_id AS text
        CONSTRAINT task_id_is_not_null
            CHECK (VALUE IS NOT NULL);

    CREATE TYPE code.worker_task AS (
        task_id        text,
        cid            text,
        task_type      text,
        task_args      jsonb,
        created_by     text,
        context        jsonb,
        ext_folder_id  text,
        hidden         boolean,
        timeout        interval,
        tracing        text
    );

    CREATE TYPE code.visibility AS ENUM (
        'visible',
        'visible+deleted',
        'all'
    );

    CREATE TYPE code.cloud AS (
        cloud_id        bigint,
        cloud_ext_id    text,
        cloud_rev       bigint,
        cpu_quota       real,
        gpu_quota       bigint,
        memory_quota    bigint,
        ssd_space_quota bigint,
        hdd_space_quota bigint,
        clusters_quota  bigint,
        cpu_used        real,
        gpu_used        bigint,
        memory_used     bigint,
        ssd_space_used  bigint,
        hdd_space_used  bigint,
        clusters_used   bigint
    );

    CREATE TYPE code.quota AS (
        cpu       real,
        memory    bigint,
        network   bigint,
        io        bigint,
        ssd_space bigint,
        hdd_space bigint,
        clusters  bigint,
        gpu       bigint
    );

    CREATE TYPE code.pillar_key AS (
        cid      text,
        subcid   text,
        shard_id text,
        fqdn     text
    );

    CREATE TYPE code.cluster_coords AS (
        cid       text,
        subcids   text[],
        shard_ids text[],
        fqdns     text[]
    );

    CREATE TYPE code.config_host AS (
        subcid          text,
        subcluster_name text,
        roles           dbaas.role_type[],
        shard_name      text,
        shard_id        text,
        fqdn            text,
        geo             text
    );

END IF;
END;
$$ 
LANGUAGE plpgsql;

