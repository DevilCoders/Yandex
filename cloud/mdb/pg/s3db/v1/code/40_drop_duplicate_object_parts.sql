/*
 * Drops duplicate object parts.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket
 * - i_name:
 *     Name of the multipart upload.
 * - i_created:
 *     Creation date of the multipart upload.
 * - i_part_id:
 *     ID of the part of the multipart upload.
 *
 * Returns:
 *   Count of deleted parts
 *
 * Raises:
 * - S3M01 (NoSuchUpload):
 *    If the requested multipart upload doesn't exist.
 * - S3M02 (InvalidPart):
 *    If the requested part doesn't exist.
 *
 * NOTE: Part duplicates should not exist.
 * After we drop all duplicates and fix s3.object_parts index
 * this function can be deleted.
 */
CREATE OR REPLACE FUNCTION v1_code.drop_duplicate_object_parts(
    i_bid uuid,
    i_name text,
    i_object_created timestamp with time zone,
    i_part_id integer
) RETURNS int
LANGUAGE plpgsql AS $$
DECLARE
    v_cid                 bigint;
    v_multipart_upload    v1_code.multipart_upload;
    v_last_created        timestamp with time zone;
    v_parts_count         integer;
    v_deleted_parts_count integer;
    v_deleted_parts_size  bigint;
    v_chunk_counters      v1_code.chunk_counters;
BEGIN
    v_cid := v1_code.get_object_cid(i_bid, i_name);

    SELECT bid, cid, name, object_created, part_id, created,
           data_size, data_md5, mds_namespace, mds_couple_id, mds_key_version,
           mds_key_uuid, storage_class, creator_id, metadata, acl
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

    SELECT max(created), count(*)
        INTO v_last_created, v_parts_count
        FROM s3.object_parts
        WHERE bid = i_bid
          AND name = i_name
          AND object_created = i_object_created
          AND part_id = i_part_id
        GROUP BY bid, name, object_created, part_id;

    IF v_parts_count = 0 OR v_parts_count IS NULL THEN
            RAISE EXCEPTION 'No such part'
                USING ERRCODE = 'S3M02';
    END IF;

    IF v_parts_count = 1 THEN
            RAISE EXCEPTION 'The part has no duplicates'
                USING ERRCODE = 'S3M02';
    END IF;

    WITH old_parts AS (
        DELETE FROM s3.object_parts
            WHERE bid = i_bid
              AND name = i_name
              AND object_created = i_object_created
              AND part_id = i_part_id
              AND created < v_last_created
            RETURNING bid, v_cid, name, object_created, part_id, created, data_size, data_md5,
                mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, metadata
    ), deleted AS (
        SELECT data_size, v1_impl.storage_delete_queue_push(
            old_parts::v1_code.object_part
            )
        FROM old_parts
        WHERE mds_couple_id IS NOT NULL
          AND mds_key_version IS NOT NULL
          AND mds_key_uuid IS NOT NULL
    )
    SELECT count(*), sum(data_size)
        INTO v_deleted_parts_count, v_deleted_parts_size
        FROM deleted;

    SELECT i_bid, v_cid,
       0 /* simple_objects_count */,
       0 /* simple_objects_size */,
       0 /* multipart_objects_count */,
       0 /* multipart_objects_size */,
       v_deleted_parts_count /* objects_parts_count */,
       v_deleted_parts_size /* objects_parts_size */,
       0 /* deleted_objects_count */,
       0 /* deleted_objects_size */,
       0 /* active_multipart_count */,
       v_multipart_upload.storage_class
    INTO v_chunk_counters;

    PERFORM v1_code.chunks_counters_queue_push(OPERATOR(v1_code.-) v_chunk_counters);

    RETURN v_deleted_parts_count;
END;
$$;
