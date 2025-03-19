/*
 * Initiates a Multipart Upload.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket.
 * - i_name:
 *     Name of the multipart upload to initiate.
 *
 * Returns:
 *   "Root" multipart upload's record -- an instance of ``code.object_part``
 *   with ``part_id`` of 0.
 */
CREATE OR REPLACE FUNCTION v1_code.start_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_mds_namespace text,
    i_mds_couple_id integer DEFAULT NULL,
    i_mds_key_version integer DEFAULT NULL,
    i_mds_key_uuid uuid DEFAULT NULL,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_lock_settings JSONB DEFAULT NULL
) RETURNS v1_code.multipart_upload
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_res v1_code.multipart_upload;
BEGIN

    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in object_parts's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_name);

    INSERT INTO s3.object_parts (
            bid, cid, name, object_created, part_id, created,
            data_size, data_md5, mds_namespace, mds_couple_id, mds_key_version,
            mds_key_uuid, storage_class, creator_id, metadata, acl, lock_settings)
        VALUES (
            i_bid, NULL, i_name, current_timestamp, 0,
            current_timestamp, 0, /*data_md5*/ NULL, i_mds_namespace, i_mds_couple_id,
            i_mds_key_version, i_mds_key_uuid, i_storage_class, i_creator_id, i_metadata, i_acl, i_lock_settings)
        RETURNING bid, cid, name, object_created, part_id, created, data_size, data_md5,
                mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, creator_id,
                metadata, acl, lock_settings
        INTO v_res;

    PERFORM v1_code.chunks_counters_queue_push(v1_code.active_multipart_get_chunk_counters(
                  i_bid, v_cid, i_storage_class,
                  0 /* parts_count */ , 0 /* parts_size */
                ));
    RETURN v_res;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.start_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_cid bigint,
    i_name text,
    i_mds_couple_id integer DEFAULT NULL,
    i_mds_key_version integer DEFAULT NULL,
    i_mds_key_uuid uuid DEFAULT NULL,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_lock_settings JSONB DEFAULT NULL
) RETURNS v1_code.multipart_upload
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_code.start_multipart_upload(
        i_bucket_name, i_bid, i_cid, i_name, /*i_mds_namespace*/ NULL,
        i_mds_couple_id, i_mds_key_version, i_mds_key_uuid, i_storage_class, i_creator_id,
        i_metadata, i_acl, i_lock_settings);
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.start_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_cid bigint,
    i_name text,
    i_mds_namespace text,
    i_mds_couple_id integer DEFAULT NULL,
    i_mds_key_version integer DEFAULT NULL,
    i_mds_key_uuid uuid DEFAULT NULL
) RETURNS v1_code.multipart_upload
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_code.start_multipart_upload(
        i_bucket_name, i_bid, i_name, i_mds_namespace,
        i_mds_couple_id, i_mds_key_version, i_mds_key_uuid, i_storage_class, i_creator_id,
        i_metadata, i_acl, i_lock_settings);
END;
$$;
