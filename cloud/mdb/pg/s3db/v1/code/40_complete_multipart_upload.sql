/*
 * Completes a Multipart Upload by "assembling" previously uploaded parts.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket.
 * - i_versioning:
 *     Versioning state of the bucket.
 * - i_name:
 *     Name of the multipart upload to complete.
 * - i_created:
 *     Creation date of the multipart upload.
 * - i_data_md5:
 *     MD5 hash of the parts data. If there's only one part ``i_data_md5`` will
 *     be MD5 hash of the part's data, otherwise ``i_data_md5`` will be MD5 hash
 *     of concatenation of textual representation of data parts MD5 hashes.
 * - i_parts_data:
 *     List of the multipart upload's parts that contains each part's ID and MD5
 *     hash of the part's data. All parts in the list must be in ascending order
 *     by part ID.
 * - i_separate_parts:
 *     Store parts in `completed_parts` table instead of the inline `parts` array
 *
 * Returns:
 *   An instance of ``code.object`` that represents added object.
 *
 * Raises:
 * - S3M01 (NoSuchUpload):
 *    If the requested multipart upload doesn't exist.
 * - S3M02 (InvalidPart):
 *    If one or more of the specified multipart upload's parts could not be
 *    found. The part might not have been uploaded, or the specified MD5 hash of
 *    the part's data might not have matched the part's actual MD5 hash.
 * - S3M03 (InvalidPartOrder):
 *    If the specified multipart upload's parts are not in ascending order.
 *
 * NOTE: On successful completion all assembled multipart upload's parts will be
 * permanently removed from ``s3.object_parts`` table.
 */
CREATE OR REPLACE FUNCTION v1_code.complete_multipart_upload(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_created timestamp with time zone,
    i_data_md5 uuid,
    i_parts_data v1_code.object_part_data[],
    i_separate_parts boolean default false
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_root_object_part v1_code.multipart_upload;
    v_object v1_code.object;
    v_multipart_parts v1_code.multipart_upload[];
    v_parts s3.object_part[];
    v_data_size bigint;
    v_parts_data v1_code.object_part_data[];
    v_parts_correct_order boolean;
    v_parts_correct_min_size boolean;
    v_root_part_metadata jsonb;
BEGIN

    SELECT (min_diff = 1 AND max_diff = 1
            AND min_part = 1 AND max_part = cnt)
        INTO v_parts_correct_order
        FROM (
            SELECT min(current - previous) AS min_diff,
                   max(current - previous) AS max_diff,
                   min(current) AS min_part,
                   max(current) AS max_part,
                   count(1) AS cnt
                FROM (
                    SELECT (p).part_id AS current,
                           lag((p).part_id, 1, 0) OVER () AS previous
                        FROM unnest(i_parts_data) p
                    ) a
            ) b;

    IF NOT v_parts_correct_order THEN
        RAISE EXCEPTION 'Multipart upload parts are in invalid order'
            USING ERRCODE = 'S3M03';
    END IF;

    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in object_parts's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_name);

    SELECT bid, cid, name, object_created, part_id, created,
           data_size, data_md5, mds_namespace, mds_couple_id, mds_key_version,
           mds_key_uuid, storage_class, creator_id, metadata, acl, lock_settings
        INTO v_root_object_part
        FROM s3.object_parts
        WHERE bid = i_bid
            AND name = i_name
            AND created = i_created
            AND part_id = 0
        FOR UPDATE;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such multipart upload'
            USING ERRCODE = 'S3M01';
    END IF;

    SELECT array_agg((bid, cid, name, object_created, part_id, created,
           data_size, data_md5, mds_namespace, mds_couple_id, mds_key_version,
           mds_key_uuid, storage_class, creator_id, metadata, acl, lock_settings)::v1_code.multipart_upload)
        INTO v_multipart_parts
    FROM (
             SELECT * FROM s3.object_parts
             WHERE bid = i_bid
               AND name = i_name
               AND object_created = i_created
               AND part_id > 0
             ORDER BY part_id
         ) a;


    SELECT array_agg((a.part_id, a.created, a.data_size,
                      a.data_md5, a.mds_couple_id, a.mds_key_version,
                      a.mds_key_uuid)::s3.object_part),
           sum(a.data_size)
           INTO v_parts, v_data_size
        FROM (
            SELECT part_id, created, data_size,
                   data_md5, mds_couple_id, mds_key_version,
                   mds_key_uuid
              FROM unnest(v_multipart_parts) p
              ORDER BY part_id
            ) a;

    SELECT array_agg((a.part_id, a.data_md5)::v1_code.object_part_data)
        INTO v_parts_data
        FROM (
            SELECT part_id, data_md5
                FROM unnest(v_parts) p
                ORDER BY part_id
            ) a;

    -- FIXME Rework this too, to remove packing/unpacking of this encryption.parts_meta
    -- This rework requires duplicating add_object logic or breaking compatibility though
    IF v_root_object_part.metadata ? 'encryption' THEN
        v_root_part_metadata := jsonb_set(v_root_object_part.metadata, '{encryption, parts_meta}', (
            SELECT jsonb_agg(a.metadata -> 'encryption' -> 'meta')
                FROM (
                    SELECT * FROM unnest(v_multipart_parts) p
                        WHERE part_id > 0
                        ORDER BY part_id
                    ) a
                )
            );
        v_root_object_part = (v_root_object_part.bid, v_root_object_part.cid,
                              v_root_object_part.name, v_root_object_part.object_created,
                              v_root_object_part.part_id, v_root_object_part.created,
                              v_root_object_part.data_size, v_root_object_part.data_md5,
                              v_root_object_part.mds_namespace, v_root_object_part.mds_couple_id,
                              v_root_object_part.mds_key_version, v_root_object_part.mds_key_uuid,
                              v_root_object_part.storage_class, v_root_object_part.creator_id,
                              v_root_part_metadata, v_root_object_part.acl,
                              v_root_object_part.lock_settings)::v1_code.multipart_upload;
    END IF;

    IF v_parts_data IS NULL OR NOT v_parts_data = i_parts_data THEN
        RAISE EXCEPTION 'One or more of the specified parts could not be found'
            USING ERRCODE = 'S3M02';
    END IF;

    /*
     * Check all parts except the last one to be at least 5MB.
     * Upper limit (5GB) on part's size is expected to be already checked by
     * ``code.upload_object_part``.
     */
    SELECT bool_and(data_size >= 5242880 OR part_id = array_length(v_parts, 1))
        INTO v_parts_correct_min_size
        FROM (
            SELECT part_id, data_size
                FROM unnest(v_parts) p
            ) a;

    IF NOT v_parts_correct_min_size THEN
        RAISE EXCEPTION 'Proposed upload is smaller than the minimum allowed object size'
            USING ERRCODE = 'S3M04';
    END IF;

    PERFORM v1_code.chunks_counters_queue_push(OPERATOR(v1_code.-) v1_code.active_multipart_get_chunk_counters(
                        i_bid, v_cid, v_root_object_part.storage_class,
                        coalesce(array_length(v_parts, 1), 0), v_data_size
                      ));

    v_object := v1_code.add_object(i_bucket_name, i_bid, i_versioning, i_name,
                                v_cid, v_data_size, i_data_md5, v_root_object_part.mds_namespace,
                                v_root_object_part.mds_couple_id, v_root_object_part.mds_key_version,
                                v_root_object_part.mds_key_uuid, v_parts, v_root_object_part.storage_class,
                                v_root_object_part.creator_id, v_root_object_part.metadata,
                                v_root_object_part.acl, i_separate_parts, v_root_object_part.lock_settings);

    DELETE FROM s3.object_parts
      WHERE bid = i_bid
        AND name = i_name
        AND object_created = i_created;

    RETURN v_object;
END;
$$;
