CREATE TYPE v1_code.account AS (
    service_id         bigint,
    status             s3.account_status_type,
    registration_date  timestamp with time zone,
    max_size           bigint,
    max_buckets        bigint,
    folder_id          text,
    cloud_id           text
);

CREATE TYPE v1_code.granted_role AS (
    service_id   bigint,
    role         s3.role_type,
    grantee_uid  bigint,
    issue_date   timestamp with time zone
);

CREATE TYPE v1_code.bucket_header AS (
    bid             uuid,
    name            text COLLATE "C",
    created         timestamp with time zone,
    versioning      s3.bucket_versioning_type,
    banned          boolean,
    service_id      bigint,
    max_size        bigint,
    anonymous_read  boolean,
    anonymous_list  boolean
);

CREATE TYPE v1_code.bucket AS (
    bid             uuid,
    name            text COLLATE "C",
    created         timestamp with time zone,
    versioning      s3.bucket_versioning_type,
    banned          boolean,
    service_id      bigint,
    max_size        bigint,
    anonymous_read  boolean,
    anonymous_list  boolean,
    website_settings JSONB,
    cors_configuration JSONB,
    default_storage_class int,
    yc_tags JSONB,
    lifecycle_rules JSONB,
    system_settings JSONB,
    encryption_settings JSONB,
    state           s3.bucket_state
);

CREATE TYPE v1_code.chunk AS (
    bid         uuid,
    cid         bigint,
    created     timestamp with time zone,
    read_only   boolean,
    start_key   text,
    end_key     text,
    shard_id    int
);

CREATE TYPE v1_code.bucket_chunk AS (
    bucket v1_code.bucket,
    chunk v1_code.chunk
);

CREATE TYPE v1_code.access_key AS (
    service_id    bigint,
    user_id       bigint,
    role          s3.role_type,
    key_id        text COLLATE "C",
    secret_token  text COLLATE "C",
    key_version   integer,
    issue_date    timestamptz
);

CREATE TYPE v1_code.bucket_stat AS (
    bid                          uuid,
    name                         text COLLATE "C",
    service_id                   bigint,
    storage_class                int,
    chunks_count                 bigint,
    simple_objects_count         bigint,
    simple_objects_size          bigint,
    multipart_objects_count      bigint,
    multipart_objects_size       bigint,
    objects_parts_count          bigint,
    objects_parts_size           bigint,
    updated_ts                   timestamptz,
    max_size                     bigint
);

CREATE TYPE v1_code.object_shard AS (
    name     text COLLATE "C",
    shard_id int
);


CREATE TYPE v1_code.enabled_lifecycle_rules AS (
    bid         uuid,
    started_ts  timestamptz,
    finished_ts timestamptz,
    rules       JSONB
);

CREATE TYPE v1_code.lifecycle_scheduling_status AS (
    key                   bigint,
    scheduling_ts         timestamptz,
    last_processed_bucket text,
    status                int
);
