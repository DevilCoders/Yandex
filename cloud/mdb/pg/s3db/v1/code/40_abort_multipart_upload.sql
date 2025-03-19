/*
 * Aborts a Multipart Upload.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket to abort multipart upload from.
 * - i_name:
 *     Name of the multipart upload to abort.
 * - i_created:
 *     Creation date of the multipart upload.
 *
 * Returns:
 *   "Root" multipart upload's record -- an instance of ``code.object_part``
 *   with ``part_id`` of 0.
 *
 * Raises:
 * - S3M01 (NoSuchUpload)
 *    If the requested multipart upload doesn't exist.
 */
CREATE OR REPLACE FUNCTION v1_code.abort_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone
) RETURNS v1_code.multipart_upload
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_multipart_upload v1_code.multipart_upload;
    v_parts_count integer;
    v_chunk_counters_change v1_code.chunk_counters;
BEGIN

    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in object_parts's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_name);

    PERFORM 1
    FROM s3.object_parts
    WHERE bid = i_bid
        AND name = i_name
        AND object_created = i_created
    FOR UPDATE;

    SELECT bid, v_cid, name, object_created, 0 as part_id,
           min(created) FILTER(WHERE part_id=0),
           coalesce(sum(data_size) FILTER(WHERE part_id>0), 0),
           (array_agg(data_md5) FILTER(WHERE part_id=0))[1],
           min(mds_namespace) FILTER(WHERE part_id=0),
           min(mds_couple_id) FILTER(WHERE part_id=0),
           min(mds_key_version) FILTER(WHERE part_id=0),
           (array_agg(mds_key_uuid) FILTER(WHERE part_id=0))[1],
           min(storage_class) FILTER(WHERE part_id=0),
           min(creator_id) FILTER(WHERE part_id=0),
           (array_agg(metadata) FILTER(WHERE part_id=0))[1],
           (array_agg(acl) FILTER(WHERE part_id=0))[1]
        INTO v_multipart_upload
        FROM s3.object_parts
      WHERE bid = i_bid
        AND name = i_name
        AND object_created = i_created
      GROUP BY bid, name, object_created;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such multipart upload'
            USING ERRCODE = 'S3M01';
    END IF;

    WITH parts AS (
        DELETE FROM s3.object_parts
            WHERE bid = i_bid
                AND name = i_name
                AND object_created = i_created
        RETURNING bid, cid, name, object_created, part_id, created, data_size,
            data_md5, mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, v_multipart_upload.storage_class, metadata
    ),
    deleted AS (
        SELECT part_id, v1_impl.storage_delete_queue_push(
            parts::v1_code.object_part
        )
        FROM parts
        WHERE mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL
            AND mds_key_uuid IS NOT NULL
    )
    SELECT count(*) INTO v_parts_count
        FROM deleted WHERE part_id > 0;

    PERFORM v1_code.chunks_counters_queue_push(OPERATOR(v1_code.-) v1_code.active_multipart_get_chunk_counters(
                        i_bid, v_cid, v_multipart_upload.storage_class,
                        v_parts_count, v_multipart_upload.data_size
                      ));

    RETURN v_multipart_upload;
END;
$$;
