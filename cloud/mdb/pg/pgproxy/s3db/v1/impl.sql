DROP SCHEMA IF EXISTS v1_impl CASCADE;
CREATE SCHEMA v1_impl;


CREATE OR REPLACE FUNCTION v1_impl.drop_accounts()
RETURNS void
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON 0;
    TARGET v1_impl.drop_accounts;
$$;

CREATE OR REPLACE FUNCTION v1_impl.get_bucket_meta_shard(
    i_name text
) RETURNS integer
LANGUAGE sql IMMUTABLE STRICT AS $function$
    SELECT hashtext(i_name);
$function$;

CREATE OR REPLACE FUNCTION v1_impl.get_chunk_shard(
    i_bucket_name text,
    i_bid uuid,
    i_cid bigint
) RETURNS integer
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON v1_impl.get_bucket_meta_shard(i_bucket_name);
    TARGET v1_code.get_chunk_shard;
$$;

CREATE OR REPLACE FUNCTION v1_impl.get_object_chunk(
    i_bucket_name text,
    i_name text,
    i_write boolean DEFAULT false
) RETURNS v1_code.chunk
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON v1_impl.get_bucket_meta_shard(i_bucket_name);
    TARGET v1_code.get_object_chunk;
$$;

CREATE OR REPLACE FUNCTION v1_impl.get_object_shard(
    i_bucket_name text,
    i_name text,
    i_write boolean DEFAULT false
) RETURNS integer
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON v1_impl.get_bucket_meta_shard(i_bucket_name);
    TARGET v1_code.get_object_shard;
$$;

CREATE OR REPLACE FUNCTION v1_impl.get_multiple_drop_object_shard(
    i_bucket_name text,
    i_multiple_drop_object v1_code.multiple_drop_object
) RETURNS integer
LANGUAGE sql AS $$
    SELECT * FROM v1_impl.get_object_shard(
        i_bucket_name, i_multiple_drop_object.name, /*i_write*/ true
    )
$$;

CREATE OR REPLACE FUNCTION v1_impl.get_bucket_chunks(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text,
    i_start_after text
) RETURNS SETOF v1_code.chunk
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON v1_impl.get_bucket_meta_shard(i_bucket_name);
    TARGET v1_code.get_bucket_chunks;
$$;

CREATE OR REPLACE FUNCTION v1_impl.get_bucket_shards(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text DEFAULT NULL,
    i_start_after text DEFAULT NULL
) RETURNS SETOF integer
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON v1_impl.get_bucket_meta_shard(i_bucket_name);
    TARGET v1_impl.get_listing_shards;
$$;


CREATE OR REPLACE FUNCTION v1_impl.list_buckets(
    i_service_id bigint DEFAULT NULL,
    i_list_all   boolean DEFAULT false
) RETURNS SETOF v1_code.bucket_header
LANGUAGE plproxy AS $$
    CLUSTER 'meta_ro';
    RUN ON ALL;
    SELECT bid, name, created, versioning, banned, service_id, max_size,
            anonymous_read, anonymous_list
        FROM v1_code.list_buckets(i_service_id, i_list_all);
$$;

CREATE OR REPLACE FUNCTION v1_impl.add_bucket(
    i_name TEXT,
    i_versioning s3.bucket_versioning_type DEFAULT 'disabled',
    i_service_id bigint DEFAULT NULL,
    i_max_size bigint DEFAULT NULL
) RETURNS v1_code.bucket_chunk
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON v1_impl.get_bucket_meta_shard(i_name);
    -- Select only known v1_code.bucket's fields
    SELECT ((bucket).bid,
            (bucket).name,
            (bucket).created,
            (bucket).versioning,
            (bucket).banned,
            (bucket).service_id,
            (bucket).max_size,
            (bucket).anonymous_read,
            (bucket).anonymous_list,
            (bucket).website_settings,
            (bucket).cors_configuration,
            (bucket).default_storage_class,
            (bucket).yc_tags,
            (bucket).lifecycle_rules,
            (bucket).system_settings,
            (bucket).encryption_settings,
            (bucket).state
            ) AS bucket,
           chunk
        FROM v1_code.add_bucket(i_name,
                                i_versioning,
                                i_service_id,
                                i_max_size);
$$;

CREATE OR REPLACE FUNCTION v1_impl.chunk_get_shard(
    i_chunk v1_code.chunk
) RETURNS int
LANGUAGE sql IMMUTABLE STRICT AS $function$
    SELECT i_chunk.shard_id;
$function$;

CREATE OR REPLACE FUNCTION v1_impl.add_chunk(
    i_bid uuid,
    i_cid bigint,
    i_start_key text,
    i_end_key text,
    i_shard_id int
) RETURNS v1_code.chunk
LANGUAGE plproxy AS $function$
    CLUSTER 'db_rw';
    RUN ON i_shard_id;
    SELECT * FROM v1_code.add_chunk(i_bid,
                                    i_cid,
                                    i_start_key,
                                    i_end_key);
$function$;

CREATE OR REPLACE FUNCTION v1_impl.add_object(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_cid bigint,
    i_data_size bigint,
    i_data_md5 uuid,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_shard_id int,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_lock_settings JSONB DEFAULT NULL
) RETURNS v1_code.object
LANGUAGE plproxy AS $$
    CLUSTER 'db_rw';
    RUN ON i_shard_id;
    SELECT bid, name, created, cid, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            null_version, delete_marker, parts_count, parts, storage_class, creator_id,
            metadata, acl
        FROM v1_code.add_object(i_bucket_name,
                             i_bid,
                             i_versioning,
                             i_name,
                             i_cid,
                             i_data_size,
                             i_data_md5,
                             i_mds_namespace,
                             i_mds_couple_id,
                             i_mds_key_version,
                             i_mds_key_uuid,
                             NULL::s3.object_part[], /* i_parts */
                             i_storage_class,
                             i_creator_id,
                             i_metadata,
                             i_acl,
                             false /* i_separate_parts */,
                             i_lock_settings
                        );
$$;

CREATE OR REPLACE FUNCTION v1_impl.add_object(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_cid bigint,
    i_data_size bigint,
    i_data_md5 uuid,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_shard_id int,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_lock_settings JSONB DEFAULT NULL
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_impl.add_object(
        i_bucket_name, i_bid, i_versioning, i_name, i_cid, i_data_size, i_data_md5,
        /*i_mds_namespace*/ NULL, i_mds_couple_id, i_mds_key_version, i_mds_key_uuid,
        i_shard_id, i_storage_class, i_creator_id, i_metadata, i_acl, i_lock_settings);
END;
$$;


CREATE OR REPLACE FUNCTION v1_impl.list_objects(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text,
    i_delimiter text,
    i_start_after text,
    i_limit integer DEFAULT 1000
) RETURNS SETOF v1_code.object
LANGUAGE plproxy AS $$
    CLUSTER 'db_ro';
    RUN ON v1_impl.get_bucket_shards(i_bucket_name, i_bid,
                                    i_prefix, i_start_after);
    SELECT bid, name, created, cid, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            null_version, delete_marker, parts_count, parts, storage_class, creator_id,
            metadata, acl, lock_settings
        FROM v1_code.list_objects(
            i_bucket_name,
            i_bid,
            i_prefix,
            i_delimiter,
            i_start_after,
            i_limit);
$$;


CREATE OR REPLACE FUNCTION v1_impl.list_multipart_uploads(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text DEFAULT NULL,
    i_delimiter text DEFAULT NULL,
    i_start_after_key text DEFAULT NULL,
    i_start_after_created timestamp with time zone DEFAULT NULL,
    i_limit integer DEFAULT 1000
) RETURNS SETOF v1_code.multipart_upload
LANGUAGE plproxy AS $$
    CLUSTER 'db_ro';
    RUN ON v1_impl.get_bucket_shards(i_bucket_name, i_bid,
                                    i_prefix, i_start_after_key);
    SELECT bid, cid, name, object_created, part_id, created, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
            storage_class, creator_id, metadata, acl, lock_settings
        FROM v1_code.list_multipart_uploads(
            i_bucket_name,
            i_bid,
            i_prefix,
            i_delimiter,
            i_start_after_key,
            i_start_after_created,
            i_limit);
$$;


CREATE OR REPLACE FUNCTION v1_impl.get_chunks_counters(
    i_bucket_name text,
    i_bid uuid,
    i_cid bigint
) RETURNS SETOF v1_code.chunk_counters
LANGUAGE plproxy AS $$
    CLUSTER 'db_ro';
    RUN ON v1_impl.get_chunk_shard(i_bucket_name, i_bid, i_cid);
    SELECT bid, cid, simple_objects_count, simple_objects_size, multipart_objects_count,
            multipart_objects_size, objects_parts_count, objects_parts_size, deleted_objects_count,
            deleted_objects_size, active_multipart_count, storage_class
        FROM v1_code.get_chunks_counters(i_bid, i_cid);
$$;


CREATE OR REPLACE FUNCTION v1_impl.drop_bucket(
    i_name text
) RETURNS void
LANGUAGE plproxy AS $$
    CLUSTER 'meta_rw';
    RUN ON v1_impl.get_bucket_meta_shard(i_name);
    TARGET v1_code.drop_bucket;
$$;


GRANT USAGE ON SCHEMA v1_impl TO s3api;
GRANT EXECUTE ON ALL functions IN SCHEMA v1_impl TO s3api;

GRANT USAGE ON SCHEMA v1_impl TO s3api_ro;
GRANT EXECUTE ON ALL functions IN SCHEMA v1_impl TO s3api_ro;

GRANT USAGE ON SCHEMA v1_impl TO s3api_list;
GRANT EXECUTE ON ALL functions IN SCHEMA v1_impl TO s3api_list;

GRANT USAGE ON SCHEMA v1_impl TO s3cleanup;
GRANT EXECUTE ON FUNCTION v1_impl.get_bucket_meta_shard(text) TO s3cleanup;
