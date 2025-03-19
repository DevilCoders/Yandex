CREATE TYPE v1_code.chunk_counters AS (
    bid             uuid,
    cid             bigint,
    simple_objects_count   bigint,
    simple_objects_size    bigint,
    multipart_objects_count   bigint,
    multipart_objects_size    bigint,
    objects_parts_count   bigint,
    objects_parts_size    bigint,
    deleted_objects_count bigint,
    deleted_objects_size  bigint,
    active_multipart_count bigint,
    storage_class    int
);

CREATE TYPE v1_code.object AS (
    bid              uuid,
    name             text COLLATE "C",
    created          timestamp with time zone,
    cid              bigint,
    data_size        bigint,
    data_md5         uuid,
    mds_namespace    text COLLATE "C",
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,
    null_version     boolean,
    delete_marker    boolean,
    parts_count      integer,
    parts            s3.object_part[],

    storage_class    int,
    creator_id       text COLLATE "C",
    metadata         JSONB,
    acl              JSONB,
    lock_settings    JSONB
);

CREATE TYPE v1_code.object_delete_marker AS (
    bid              uuid,
    name             text COLLATE "C",
    created          timestamp with time zone,
    creator_id       text COLLATE "C",
    null_version     boolean
);

CREATE TYPE v1_code.object_version AS (
    bid              uuid,
    name             text COLLATE "C",
    created          timestamp with time zone,
    noncurrent       timestamp with time zone,
    data_size        bigint,
    data_md5         uuid,
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,
    null_version     boolean,
    delete_marker    boolean,
    parts_count      integer,
    parts            s3.object_part[],
    storage_class    int,
    creator_id       text COLLATE "C",
    metadata         JSONB,
    acl              JSONB,
    lock_settings    JSONB
);

CREATE TYPE v1_code.object_part AS (
    bid              uuid,
    cid              bigint,
    name             text COLLATE "C",
    object_created   timestamp with time zone,
    part_id          integer,
    created          timestamp with time zone,
    data_size        bigint,
    data_md5         uuid,
    mds_namespace    text COLLATE "C",
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,
    storage_class    int,
    metadata         JSONB
);

CREATE TYPE v1_code.multipart_upload AS (
    bid              uuid,
    cid              bigint,
    name             text COLLATE "C",
    object_created   timestamp with time zone,
    part_id          integer,
    created          timestamp with time zone,
    data_size        bigint,
    data_md5         uuid,
    mds_namespace    text COLLATE "C",
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,

    storage_class    int,
    creator_id       text COLLATE "C",
    metadata         JSONB,
    acl              JSONB,
    lock_settings    JSONB
);

CREATE TYPE v1_code.object_part_data AS (
    part_id         integer,
    data_md5        uuid
);

CREATE TYPE v1_code.multiple_drop_object AS (
    name            text COLLATE "C",
    created         timestamp with time zone
);

CREATE TYPE v1_code.multiple_drop_object_result AS (
    name                   text COLLATE "C",
    created                timestamp with time zone,
    delete_marker          boolean,
    delete_marker_created  timestamp with time zone,
    error                  text COLLATE "C"
);

CREATE TYPE v1_code.deleted_object AS (
    bid              uuid,
    name             text COLLATE "C",
    part_id          integer,
    data_size        bigint,
    mds_namespace    text COLLATE "C",
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,
    remove_after_ts  timestamp with time zone,
    storage_class    int
);

CREATE TYPE v1_code.chunk AS (
    bid uuid,
    cid bigint,
    created timestamp with time zone,
    read_only boolean,
    start_key text,
    end_key text,
    shard_id int
);

CREATE TYPE v1_code.list_name_item AS (
    bid uuid,
    name text COLLATE "C",
    is_prefix boolean,
    idx integer
);

CREATE TYPE v1_code.lifecycle_element_key AS (
    name            text COLLATE "C",
    created         timestamp with time zone
);

CREATE TYPE v1_code.lifecycle_result AS (
    name            text COLLATE "C",
    created         timestamp with time zone,
    error           text COLLATE "C"
);

/* Struct to represent result of inflight operations, e.g.:
 * ``v1_code.start_inflight_upload``
 * ``v1_code.add_inflight_upload``
 */
CREATE TYPE v1_code.inflight_result AS (
    bid               uuid,
    object_name       text,
    object_created    timestamp with time zone,
    inflight_created  timestamp with time zone,

    part_id          integer,

    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,

    -- Reserved
    metadata JSONB
);

CREATE TYPE v1_code.completed_part AS (
    bid             uuid,
    name            text COLLATE "C",
    object_created  timestamptz,
    part_id         integer,
    end_offset      bigint,
    created         timestamptz,
    data_size       bigint,
    data_md5        uuid,
    mds_couple_id   integer,
    mds_key_version integer,
    mds_key_uuid    uuid,
    storage_class   int,
    encryption      text COLLATE "C"
);

-- This structure is returned from object_info_with_parts()
-- Either part fields or object fields are filled in it (like in outer join)
-- FIXME: Think about leaving only part_id, part_end_offset and part_encryption
-- and using object fields for the rest because object and part fields are similar
CREATE TYPE v1_code.object_with_part AS (
    bid              uuid,
    name             text COLLATE "C",
    created          timestamp with time zone,
    noncurrent       timestamp with time zone,
    data_size        bigint,
    data_md5         uuid,
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,
    null_version     boolean,
    delete_marker    boolean,
    parts_count      integer,
    storage_class    int,
    creator_id       text COLLATE "C",
    metadata         JSONB,
    acl              JSONB,
    lock_settings    JSONB,

    part_id               integer,
    part_end_offset       bigint,
    part_created          timestamp with time zone,
    part_data_size        bigint,
    part_data_md5         uuid,
    part_mds_couple_id    integer,
    part_mds_key_version  integer,
    part_mds_key_uuid     uuid,
    part_storage_class    int,
    part_encryption       text COLLATE "C"
);

CREATE TYPE v1_code.composite_object_info AS (
    bid              uuid,
    name             text COLLATE "C",
    created          timestamp with time zone,
    noncurrent       timestamp with time zone,
    data_size        bigint,
    data_md5         uuid,
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,
    null_version     boolean,
    delete_marker    boolean,
    parts_count      integer,
    storage_class    int,
    creator_id       text COLLATE "C",
    metadata         JSONB,
    acl              JSONB,
    lock_settings    JSONB
);

CREATE TYPE v1_code.composite_part_info AS (
    part_id               integer,
    part_end_offset       bigint,
    part_created          timestamp with time zone,
    part_data_size        bigint,
    part_data_md5         uuid,
    part_mds_couple_id    integer,
    part_mds_key_version  integer,
    part_mds_key_uuid     uuid,
    part_storage_class    int,
    part_encryption       text COLLATE "C"
);
