DROP SCHEMA IF EXISTS v1_code CASCADE;
CREATE SCHEMA v1_code;

DROP SCHEMA IF EXISTS s3 CASCADE;
CREATE SCHEMA s3;

CREATE TYPE s3.account_status_type AS ENUM (
    'active',
    'suspended'
);

CREATE TYPE s3.role_type AS ENUM (
    'admin',
    'owner',
    'reader',
    'uploader',
    'writer',
    'presigner',
    'public_manager',
    'private_manager',
    'any',
    'staff_manager'
);

CREATE TYPE s3.bucket_versioning_type AS ENUM (
    'disabled',
    'enabled',
    'suspended'
);

CREATE TYPE s3.bucket_state AS ENUM (
    'alive',
    'deleting'
);

CREATE TYPE s3.object_part AS (
    part_id          integer,
    created          timestamp with time zone,
    data_size        bigint,
    data_md5         uuid,
    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid
);

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

CREATE TYPE v1_code.access_key AS (
    service_id    bigint,
    user_id       bigint,
    role          s3.role_type,
    key_id        text COLLATE "C",
    secret_token  text COLLATE "C",
    key_version   integer,
    issue_date    timestamptz
);

CREATE TYPE v1_code.bucket_header AS (
    bid uuid,
    name TEXT COLLATE "C",
    created timestamp with time zone,
    versioning s3.bucket_versioning_type,
    banned boolean,
    service_id bigint,
    max_size bigint,
    anonymous_read boolean,
    anonymous_list boolean
);

CREATE TYPE v1_code.bucket AS (
    bid uuid,
    name TEXT COLLATE "C",
    created timestamp with time zone,
    versioning s3.bucket_versioning_type,
    banned boolean,
    service_id bigint,
    max_size bigint,
    anonymous_read boolean,
    anonymous_list boolean,
    website_settings JSONB,
    cors_configuration JSONB,
    default_storage_class int,
    yc_tags JSONB,
    lifecycle_rules JSONB,
    system_settings JSONB,
    encryption_settings JSONB,
    state s3.bucket_state
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

CREATE TYPE v1_code.bucket_chunk AS (
    bucket v1_code.bucket,
    chunk v1_code.chunk
);

CREATE TYPE v1_code.chunk_counters AS (
    bid uuid,
    cid bigint,
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

CREATE OR REPLACE FUNCTION v1_code.register_account(
    i_service_id bigint,
    i_max_size bigint DEFAULT NULL,
    i_max_buckets bigint DEFAULT NULL
) RETURNS v1_code.account
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_code.register_account;
$$;

CREATE OR REPLACE FUNCTION v1_code.suspend_account(
    i_service_id bigint
) RETURNS v1_code.account
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_code.suspend_account;
$$;

CREATE OR REPLACE FUNCTION v1_code.account_info(
    i_service_id bigint
) RETURNS v1_code.account
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON 0;
    SELECT service_id, status, registration_date, max_size, max_buckets, cloud_id, folder_id
        FROM v1_code.account_info(i_service_id);
$$;

/*
 * TODO: Add support to list only 'active' accounts.
 */
CREATE OR REPLACE FUNCTION v1_code.list_accounts()
RETURNS SETOF v1_code.account
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON 0;
    SELECT service_id, status, registration_date, max_size, max_buckets, cloud_id, folder_id
        FROM s3.accounts;
$$;


CREATE OR REPLACE FUNCTION v1_code.grant_role(
    i_service_id bigint,
    i_role s3.role_type,
    i_grantee_uid bigint
) RETURNS v1_code.granted_role
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_code.grant_role;
$$;

CREATE OR REPLACE FUNCTION v1_code.tvm2_grant_role(
    i_service_id bigint,
    i_role s3.role_type,
    i_grantee_uid bigint
) RETURNS v1_code.granted_role
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_code.tvm2_grant_role;
$$;


CREATE OR REPLACE FUNCTION v1_code.remove_granted_role(
    i_service_id bigint,
    i_role s3.role_type,
    i_grantee_uid bigint
) RETURNS v1_code.granted_role
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_code.remove_granted_role;
$$;

CREATE OR REPLACE FUNCTION v1_code.remove_tvm2_granted_role(
    i_service_id bigint,
    i_role s3.role_type,
    i_grantee_uid bigint
) RETURNS v1_code.granted_role
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_code.remove_tvm2_granted_role;
$$;


CREATE OR REPLACE FUNCTION v1_code.list_granted_roles()
RETURNS SETOF v1_code.granted_role
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON 0;
    SELECT service_id, role, grantee_uid, issue_date
        FROM s3.granted_roles;
$$;

CREATE OR REPLACE FUNCTION v1_code.list_tvm2_granted_roles()
RETURNS SETOF v1_code.granted_role
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON 0;
    SELECT service_id, role, grantee_uid, issue_date
        FROM s3.tvm2_granted_roles;
$$;


CREATE OR REPLACE FUNCTION v1_code.add_access_key(
    i_service_id bigint,
    i_user_id bigint,
    i_role s3.role_type,
    i_key_id text,
    i_secret_token text,
    i_key_version integer
) RETURNS v1_code.access_key
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_code.add_access_key;
$$;

CREATE OR REPLACE FUNCTION v1_code.delete_access_key(
    i_service_id bigint,
    i_user_id bigint,
    i_role s3.role_type,
    i_key_id text
) RETURNS v1_code.access_key
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_code.delete_access_key;
$$;

CREATE OR REPLACE FUNCTION v1_code.list_access_keys()
RETURNS SETOF v1_code.access_key
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON 0;
    TARGET v1_code.list_access_keys;
$$;


CREATE OR REPLACE FUNCTION v1_code.list_buckets(
    i_service_id bigint DEFAULT NULL,
    i_list_all   boolean DEFAULT false
) RETURNS SETOF v1_code.bucket_header
LANGUAGE plpgsql
AS $$
BEGIN
    RETURN QUERY
    SELECT DISTINCT bid, name, created, versioning, banned, service_id, max_size,
            anonymous_read, anonymous_list
        FROM v1_impl.list_buckets(i_service_id, i_list_all)
        ORDER BY name;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.add_bucket(
    i_name TEXT,
    i_versioning s3.bucket_versioning_type DEFAULT 'disabled',
    i_service_id bigint DEFAULT NULL,
    i_max_size bigint DEFAULT NULL
) RETURNS v1_code.bucket
LANGUAGE plpgsql AS $$
DECLARE
    v_bucket_chunk v1_code.bucket_chunk;
    v_chunk v1_code.chunk;
BEGIN
    SELECT * INTO v_bucket_chunk FROM v1_impl.add_bucket(i_name,
                                                        i_versioning,
                                                        i_service_id,
                                                        i_max_size
                                                      );
    RAISE NOTICE 'Chunk %', (v_bucket_chunk.chunk).shard_id;
    v_chunk := v_bucket_chunk.chunk;
    PERFORM v1_impl.add_chunk(v_chunk.bid,
                            v_chunk.cid,
                            v_chunk.start_key,
                            v_chunk.end_key,
                            v_chunk.shard_id
                        );

    RETURN v_bucket_chunk.bucket;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.bucket_info(
    i_name text
) RETURNS v1_code.bucket
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON v1_impl.get_bucket_meta_shard(i_name);
    SELECT bid, name, created, versioning, banned, service_id, max_size,
            anonymous_read, anonymous_list, website_settings, cors_configuration,
            default_storage_class, yc_tags, lifecycle_rules, system_settings, encryption_settings, state
        FROM v1_code.bucket_info(i_name);
$$;

CREATE OR REPLACE FUNCTION v1_code.bid_info(
    i_bid uuid
) RETURNS SETOF v1_code.bucket
ROWS 1
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON ALL;
    SELECT bid, name, created, versioning, banned, service_id, max_size,
            anonymous_read, anonymous_list, website_settings, cors_configuration,
            default_storage_class, yc_tags, lifecycle_rules, system_settings, encryption_settings
        FROM s3.buckets
        WHERE bid = i_bid;
$$;

CREATE OR REPLACE FUNCTION v1_code.get_buckets_stats()
RETURNS SETOF v1_code.bucket_stat
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON ALL;
    SELECT bid, name, service_id, storage_class, chunks_count, simple_objects_count,
            simple_objects_size, multipart_objects_count, multipart_objects_size,
            objects_parts_count, objects_parts_size, updated_ts, max_size
        FROM v1_code.get_buckets_stats();
$$;

CREATE OR REPLACE FUNCTION v1_code.get_bucket_stat(
    i_bucket_name text
) RETURNS v1_code.bucket_stat
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON v1_impl.get_bucket_meta_shard(i_bucket_name);
    SELECT bid, name, service_id, storage_class, chunks_count, simple_objects_count,
            simple_objects_size, multipart_objects_count, multipart_objects_size,
            objects_parts_count, objects_parts_size, updated_ts, max_size
        FROM v1_code.get_bucket_stat(i_bucket_name);
$$;

CREATE OR REPLACE FUNCTION v1_code.get_service_buckets_stats(
    i_service_id bigint
) RETURNS SETOF v1_code.bucket_stat
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON ALL;
    SELECT bid, name, service_id, storage_class, chunks_count, simple_objects_count,
            simple_objects_size, multipart_objects_count, multipart_objects_size,
            objects_parts_count, objects_parts_size, updated_ts, max_size
        FROM v1_code.get_service_buckets_stats(i_service_id);
$$;


CREATE OR REPLACE FUNCTION v1_code.modify_bucket(
    i_name TEXT,
    i_versioning s3.bucket_versioning_type,
    i_banned boolean,
    i_max_size bigint DEFAULT NULL,
    i_anonymous_read boolean DEFAULT NULL,
    i_anonymous_list boolean DEFAULT NULL,
    i_website_settings JSONB DEFAULT NULL,
    i_cors_configuration JSONB DEFAULT NULL,
    i_default_storage_class int DEFAULT NULL,
    i_yc_tags JSONB DEFAULT NULL,
    i_lifecycle_rules JSONB DEFAULT NULL,
    i_system_settings JSONB DEFAULT NULL,
    i_service_id bigint DEFAULT NULL
) RETURNS v1_code.bucket
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON v1_impl.get_bucket_meta_shard(i_name);
    SELECT bid, name, created, versioning, banned, service_id, max_size,
            anonymous_read, anonymous_list, website_settings,
            cors_configuration, default_storage_class, yc_tags,
            lifecycle_rules, system_settings, encryption_settings, state
        FROM v1_code.modify_bucket(i_name,
                                   i_versioning,
                                   i_banned,
                                   i_max_size,
                                   i_anonymous_read,
                                   i_anonymous_list,
                                   i_website_settings,
                                   i_cors_configuration,
                                   i_default_storage_class,
                                   i_yc_tags,
                                   i_lifecycle_rules,
                                   i_system_settings,
                                   i_service_id);
$$;

CREATE OR REPLACE FUNCTION v1_code.drop_bucket(
    i_name text
) RETURNS void
LANGUAGE plpgsql AS $$
DECLARE
    v_bid uuid;
BEGIN
    SELECT bid INTO v_bid FROM v1_code.bucket_info(i_name);

    -- Check that bucket has no objects
    PERFORM 1 FROM v1_code.list_objects(
        i_name, v_bid, '' /* i_prefix */,
        NULL /* i_delimiter */,
        '' /* i_start_after */,
        1 /* i_limit */);
    IF FOUND THEN
        RAISE EXCEPTION 'Could not delete bucket with objects'
            USING ERRCODE = 'S3B06';
    END IF;

    -- Check that bucket has no incomplete multipart uploads
    PERFORM 1 FROM v1_code.list_multipart_uploads(
        i_name, v_bid, '' /* i_prefix */,
        NULL /* i_delimiter */,
        '' /* i_start_after_key */,
        NULL /* i_start_after_created */,
        1 /* i_limit */);
    IF FOUND THEN
        RAISE EXCEPTION 'Could not delete bucket with incomplete multipart uploads'
            USING ERRCODE = 'S3B07';
    END IF;

    PERFORM v1_impl.drop_bucket(i_name);
END;
$$;


CREATE OR REPLACE FUNCTION v1_code.add_object(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_data_size bigint,
    i_data_md5 uuid,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL
) RETURNS v1_code.object
LANGUAGE plpgsql
AS $$
DECLARE
    v_chunk v1_code.chunk;
    v_object v1_code.object;
BEGIN
    -- NOTE: v1_impl.get_object_chunk will raise an exception if no appropriate
    -- chunk was found
    SELECT * INTO v_chunk
        FROM v1_impl.get_object_chunk(i_bucket_name,
                                   i_name,
                                   /*i_write*/ true);

    SELECT bid, name, created, cid, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            null_version, delete_marker, parts_count, parts, storage_class,
            creator_id, metadata, acl
        INTO v_object
        FROM v1_impl.add_object(i_bucket_name,
                             i_bid,
                             i_versioning,
                             i_name,
                             v_chunk.cid,
                             i_data_size,
                             i_data_md5,
                             i_mds_namespace,
                             i_mds_couple_id,
                             i_mds_key_version,
                             i_mds_key_uuid,
                             v_chunk.shard_id,
                             i_storage_class,
                             i_creator_id,
                             i_metadata,
                             i_acl);
    RETURN v_object;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.add_object(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_data_size bigint,
    i_data_md5 uuid,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_code.add_object(
        i_bucket_name, i_bid, i_versioning, i_name, i_data_size, i_data_md5,
        /*i_mds_namespace*/ NULL, i_mds_couple_id, i_mds_key_version, i_mds_key_uuid,
        i_storage_class, i_creator_id, i_metadata, i_acl);
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.drop_object(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_created timestamp with time zone DEFAULT NULL
) RETURNS SETOF v1_code.object
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ true);
    SELECT bid, name, created, cid, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            null_version, delete_marker, parts_count, parts, storage_class,
            creator_id, metadata, acl, lock_settings
        FROM v1_code.drop_object(i_bid, i_versioning, i_name, i_created);
$$;

CREATE OR REPLACE FUNCTION v1_code.drop_multiple_objects(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_multiple_drop_objects v1_code.multiple_drop_object[]
) RETURNS SETOF v1_code.multiple_drop_object_result
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    SPLIT i_multiple_drop_objects;
    RUN ON v1_impl.get_multiple_drop_object_shard(i_bucket_name, i_multiple_drop_objects);
    SELECT * FROM v1_code.drop_multiple_objects(i_bid, i_versioning, i_multiple_drop_objects);
$$;

CREATE OR REPLACE FUNCTION v1_code.update_object_metadata(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_created timestamp with time zone DEFAULT NULL,
    i_update_modified BOOLEAN DEFAULT TRUE
) RETURNS v1_code.object
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ true);
    SELECT bid, name, created, cid, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            null_version, delete_marker, parts_count, parts, storage_class,
            creator_id, metadata, acl, lock_settings
        FROM v1_code.update_object_metadata(
            i_bucket_name,
            i_bid,
            i_name,
            i_mds_namespace,
            i_mds_couple_id,
            i_mds_key_version,
            i_mds_key_uuid,
            i_storage_class,
            i_creator_id,
            i_metadata,
            i_acl,
            i_created,
            i_update_modified
        );
$$;

CREATE OR REPLACE FUNCTION v1_code.update_object_metadata(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_mds_couple_id integer DEFAULT NULL,
    i_mds_key_version integer DEFAULT NULL,
    i_mds_key_uuid uuid DEFAULT NULL,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_created timestamp with time zone DEFAULT NULL,
    i_update_modified BOOLEAN DEFAULT TRUE
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_code.update_object_metadata(
            i_bucket_name, i_bid, i_name, /*i_mds_namespace*/ NULL,
            i_mds_couple_id, i_mds_key_version, i_mds_key_uuid, i_storage_class,
            i_creator_id, i_metadata, i_acl, i_created, i_update_modified);
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.list_objects(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text,
    i_delimiter text,
    i_start_after text,
    i_limit integer DEFAULT 1000
) RETURNS SETOF v1_code.object
LANGUAGE plpgsql STABLE AS $$
BEGIN
    RETURN QUERY
        SELECT DISTINCT
                bid, name, created, cid, data_size, data_md5,
                mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
                null_version, delete_marker, parts_count, parts, storage_class, creator_id,
                metadata, acl, lock_settings
            FROM v1_impl.list_objects(
                i_bucket_name,
                i_bid,
                i_prefix,
                i_delimiter,
                i_start_after,
                i_limit) object
        ORDER BY object.name
        LIMIT i_limit;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.object_info(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone DEFAULT NULL,
    i_null_version boolean DEFAULT false
) RETURNS SETOF v1_code.object
LANGUAGE plproxy AS $$
    CLUSTER 'db_ro';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ false);
    SELECT bid, name, created, cid, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            null_version, delete_marker, parts_count, parts, storage_class,
            creator_id, metadata, acl
        FROM v1_code.object_info(
            i_bucket_name,
            i_bid,
            i_name,
            i_created,
            i_null_version
        );
$$;

CREATE OR REPLACE FUNCTION v1_code.actual_object_info(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone DEFAULT NULL,
    i_null_version boolean DEFAULT false
) RETURNS SETOF v1_code.object
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ false);
    SELECT bid, name, created, cid, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            null_version, delete_marker, parts_count, parts, storage_class,
            creator_id, metadata, acl
        FROM v1_code.object_info(
            i_bucket_name,
            i_bid,
            i_name,
            i_created,
            i_null_version
        );
$$;


CREATE OR REPLACE FUNCTION v1_code.start_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL
) RETURNS v1_code.multipart_upload
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ true);
    SELECT bid, cid, name, object_created, part_id, created, data_size, data_md5,
        mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class,
        creator_id, metadata, acl
      FROM v1_code.start_multipart_upload(
          i_bucket_name,
          i_bid,
          i_name,
          i_mds_namespace,
          i_mds_couple_id,
          i_mds_key_version,
          i_mds_key_uuid,
          i_storage_class,
          i_creator_id,
          i_metadata,
          i_acl);
$$;

CREATE OR REPLACE FUNCTION v1_code.start_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_mds_couple_id integer DEFAULT NULL,
    i_mds_key_version integer DEFAULT NULL,
    i_mds_key_uuid uuid DEFAULT NULL,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL
) RETURNS v1_code.multipart_upload
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_code.start_multipart_upload(
        i_bucket_name, i_bid, i_name, /*i_mds_namespace*/ NULL,
        i_mds_couple_id, i_mds_key_version, i_mds_key_uuid, i_storage_class,
        i_creator_id, i_metadata, i_acl);
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.abort_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone
) RETURNS v1_code.multipart_upload
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ true);
    SELECT bid, cid, name, object_created, part_id, created, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            storage_class, creator_id, metadata, acl
        FROM v1_code.abort_multipart_upload(
            i_bucket_name,
            i_bid,
            i_name,
            i_created);
$$;

CREATE OR REPLACE FUNCTION v1_code.complete_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_created timestamp with time zone,
    i_data_md5 uuid,
    i_parts_data v1_code.object_part_data[]
) RETURNS v1_code.object
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ true);
    SELECT bid, name, created, cid, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            null_version, delete_marker, parts_count, parts, storage_class,
            creator_id, metadata, acl
        FROM v1_code.complete_multipart_upload(
            i_bucket_name,
            i_bid,
            i_versioning,
            i_name,
            i_created,
            i_data_md5,
            i_parts_data);
$$;

CREATE OR REPLACE FUNCTION v1_code.upload_object_part(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_object_created timestamp with time zone,
    i_part_id integer,
    i_data_size bigint,
    i_data_md5 uuid,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_metadata JSONB DEFAULT NULL
) RETURNS v1_code.object_part
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ true);
    SELECT bid, cid, name, object_created, part_id, created, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, metadata
        FROM v1_code.upload_object_part(
            i_bucket_name,
            i_bid,
            i_name,
            i_object_created,
            i_part_id,
            i_data_size,
            i_data_md5,
            i_mds_namespace,
            i_mds_couple_id,
            i_mds_key_version,
            i_mds_key_uuid,
            i_metadata);
$$;

CREATE OR REPLACE FUNCTION v1_code.upload_object_part(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_object_created timestamp with time zone,
    i_part_id integer,
    i_data_size bigint,
    i_data_md5 uuid,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_metadata JSONB DEFAULT NULL
) RETURNS v1_code.object_part
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_code.upload_object_part(
        i_bucket_name, i_bid, i_name, i_object_created, i_part_id, i_data_size, i_data_md5,
        /*i_mds_namespace*/ NULL, i_mds_couple_id, i_mds_key_version, i_mds_key_uuid, storage_class, i_metadata);
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.list_multipart_uploads(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text DEFAULT NULL,
    i_delimiter text DEFAULT NULL,
    i_start_after_key text DEFAULT NULL,
    i_start_after_created timestamp with time zone DEFAULT NULL,
    i_limit integer DEFAULT 1000
) RETURNS SETOF v1_code.multipart_upload
LANGUAGE plpgsql STABLE AS $$
BEGIN
    RETURN QUERY
        SELECT DISTINCT
                bid, cid, name, object_created, part_id, created, data_size, data_md5,
                mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, creator_id,
                metadata, acl, lock_settings
            FROM v1_impl.list_multipart_uploads(
                i_bucket_name,
                i_bid,
                i_prefix,
                i_delimiter,
                i_start_after_key,
                i_start_after_created,
                i_limit) object_part
        ORDER BY object_part.name
        LIMIT i_limit;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.list_current_parts(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone,
    i_start_part integer DEFAULT 1,
    i_limit integer DEFAULT 10
) RETURNS SETOF v1_code.object_part
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ false);
    SELECT bid, cid, name, object_created, part_id, created, data_size, data_md5,
           mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, metadata
        FROM s3.object_parts
      WHERE bid = i_bid
        AND name = i_name
        AND object_created = i_created
        AND part_id >= i_start_part
      ORDER BY part_id
      LIMIT i_limit;
$$;

CREATE OR REPLACE FUNCTION v1_code.multipart_upload_info(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone
) RETURNS SETOF v1_code.multipart_upload
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON v1_impl.get_object_shard(i_bucket_name, i_name, /*i_write*/ false);
    SELECT bid, cid, name, object_created, part_id, created, data_size, data_md5,
           mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class,
           creator_id, metadata, acl
        FROM v1_code.multipart_upload_info(i_bucket_name, i_bid, i_name, i_created)
$$;

CREATE OR REPLACE FUNCTION v1_code.get_chunks_counters(
    i_bucket_name text,
    i_bid uuid,
    i_cid bigint DEFAULT NULL
) RETURNS SETOF v1_code.chunk_counters
LANGUAGE plpgsql AS $$
DECLARE
    v_chunk v1_code.chunk;
BEGIN
    IF i_cid IS NOT NULL THEN
        RETURN QUERY SELECT bid, cid, simple_objects_count, simple_objects_size, multipart_objects_count,
          multipart_objects_size, objects_parts_count, objects_parts_size, deleted_objects_count,
          deleted_objects_size, active_multipart_count, storage_class
            FROM v1_impl.get_chunks_counters(i_bucket_name, i_bid, i_cid);
        RETURN;
    END IF;

    FOR v_chunk IN
        SELECT chunk.*
            FROM v1_impl.get_bucket_chunks(i_bucket_name,
                                        i_bid,
                                        /* i_prefix */ NULL,
                                        /* i_start_after */ NULL) chunk
    LOOP
        RETURN QUERY SELECT bid, cid, simple_objects_count, simple_objects_size, multipart_objects_count,
          multipart_objects_size, objects_parts_count, objects_parts_size, deleted_objects_count,
          deleted_objects_size, active_multipart_count, storage_class
            FROM v1_impl.get_chunks_counters(i_bucket_name, v_chunk.bid, v_chunk.cid);
    END LOOP;
    RETURN;
END;
$$;

GRANT USAGE ON SCHEMA v1_code TO s3api;
GRANT EXECUTE ON ALL functions IN SCHEMA v1_code TO s3api;
GRANT USAGE ON SCHEMA s3 TO s3api;

GRANT USAGE ON SCHEMA v1_code TO s3api_ro;
GRANT EXECUTE ON ALL functions IN SCHEMA v1_code TO s3api_ro;
GRANT USAGE ON SCHEMA s3 TO s3api_ro;

GRANT USAGE ON SCHEMA v1_code TO s3api_list;
GRANT EXECUTE ON ALL functions IN SCHEMA v1_code TO s3api_list;
GRANT USAGE ON SCHEMA s3 TO s3api_list;

GRANT USAGE ON SCHEMA v1_code TO s3cleanup;
GRANT EXECUTE ON FUNCTION v1_code.bucket_info(text) TO s3cleanup;
