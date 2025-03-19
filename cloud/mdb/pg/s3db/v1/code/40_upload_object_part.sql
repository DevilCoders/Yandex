/*
 * Adds a part of a Multipart Upload.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket to add the multipart upload's part to.
 * - i_name:
 *     Name of the multipart upload.
 * - i_object_created:
 *     Creation date of the multipart upload.
 * - i_part_id:
       ID of the part of the multipart upload to add.
 * - i_data_size, i_data_md5:
 *     Size and MD5 hash of the multipart upload part's data.
 * - i_mds_couple_id, i_mds_key_version, i_mds_key_uuid:
 *     MDS storage information of the stored multipart upload part's data.
 *
 * Returns:
 *   An instance of ``code.object_part`` that represents added multipart upload
 *   part.
 *
 * Raises:
 * - S3M01 (NoSuchUpload):
 *    If the requested multipart upload doesn't exist.
 */
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
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_multipart_upload v1_code.multipart_upload;
    v_current_part      v1_code.object_part;
    v_removed_part      v1_code.object_part;
    v_uploaded_part     v1_code.object_part;
    v_chunk_counters_change v1_code.chunk_counters;
    v_removed_part_size bigint := 0;
BEGIN

    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in object_parts's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_name);

    SELECT bid, cid, name, object_created, part_id, created,
           data_size, data_md5, mds_namespace, mds_couple_id, mds_key_version,
           mds_key_uuid, storage_class, creator_id, metadata, acl, lock_settings
        INTO v_multipart_upload
        FROM s3.object_parts
      WHERE bid = i_bid
        AND name = i_name
        AND created = i_object_created
        AND part_id = 0
      FOR KEY SHARE;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such multipart upload'
            USING ERRCODE = 'S3M01';
    END IF;

    /*
     * Multipart object part's sizet is limited by 5 gigabytes. Check its size
     * here to raise appropriate exception instead of generic ``check_violation``
     * error.
     */
    IF i_data_size > 5368709120 THEN
        RAISE EXCEPTION 'Object part is too large'
            USING ERRCODE = 'S3M05';
    END IF;

    SELECT bid, v_cid, name, object_created, part_id, created,
            data_size, data_md5, mds_namespace, mds_couple_id, mds_key_version,
            mds_key_uuid, storage_class, metadata
        INTO v_current_part
        FROM s3.object_parts
        WHERE bid = i_bid
          AND name = i_name
          AND object_created = i_object_created
          AND part_id = i_part_id;

    IF FOUND THEN
        /*
         * NOTE: Due to connectivity issues client could receive error, while
         * transaction was actually committed, and retry the request with the same
         * arguments.
         *
         * Here, we check whether the current version of the part is the same as
         * requested and respond with the current version if it is.
         */
        IF v_current_part.data_size = i_data_size
            AND v_current_part.data_md5 = i_data_md5
            AND v_current_part.mds_namespace IS NOT DISTINCT FROM i_mds_namespace
            AND v_current_part.mds_couple_id = i_mds_couple_id
            AND v_current_part.mds_key_version = i_mds_key_version
            AND v_current_part.mds_key_uuid = i_mds_key_uuid
            AND v_current_part.metadata IS NOT DISTINCT FROM i_metadata
        THEN
            /*
             * Due to PostgreSQL synchronous replication feature we need to perform fake
             * update of this row to make sure that we are not isolated master.
             */
            UPDATE s3.object_parts
                SET created = created
                WHERE bid = i_bid
                    AND name = i_name
                    AND object_created = i_object_created
                    AND part_id = i_part_id;
            RETURN v_current_part;
        END IF;

        -- We should rewrite previous versions of same part
        UPDATE s3.object_parts
            SET created = current_timestamp,
                data_size = i_data_size,
                data_md5 = i_data_md5,
                mds_namespace = i_mds_namespace,
                mds_couple_id = i_mds_couple_id,
                mds_key_version = i_mds_key_version,
                mds_key_uuid = i_mds_key_uuid,
                metadata = i_metadata
          WHERE bid = i_bid
            AND name = i_name
            AND object_created = i_object_created
            AND part_id = i_part_id
            AND created != current_timestamp
        RETURNING bid, v_cid, name, object_created, part_id, created, data_size, data_md5,
                mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, metadata
            INTO v_uploaded_part;

        v_chunk_counters_change := v_chunk_counters_change OPERATOR(v1_code.-) (v_current_part);

        PERFORM v1_impl.storage_delete_queue_push(v_current_part);
    ELSE
        -- Insert new part
        INSERT INTO s3.object_parts (
                bid, cid, name, object_created, part_id, created,
                data_size, data_md5, mds_namespace, mds_couple_id, mds_key_version,
                mds_key_uuid, storage_class, metadata)
            VALUES (
                i_bid, NULL, i_name, i_object_created, i_part_id,
                current_timestamp, i_data_size, i_data_md5, i_mds_namespace, i_mds_couple_id,
                i_mds_key_version, i_mds_key_uuid, v_multipart_upload.storage_class, i_metadata)
            RETURNING bid, v_cid, name, object_created, part_id, created, data_size, data_md5,
                    mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, metadata
            INTO v_uploaded_part;
    END IF;

    v_chunk_counters_change := v_chunk_counters_change OPERATOR(v1_code.+) v_uploaded_part;

    PERFORM v1_code.chunks_counters_queue_push(v_chunk_counters_change);

    RETURN v_uploaded_part;
END;
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
        i_bucket_name, i_bid, i_name, i_object_created, i_part_id, i_data_size,
        i_data_md5, /*i_mds_namespace*/ NULL, i_mds_couple_id, i_mds_key_version,
        i_mds_key_uuid, i_metadata);
END;
$$;
