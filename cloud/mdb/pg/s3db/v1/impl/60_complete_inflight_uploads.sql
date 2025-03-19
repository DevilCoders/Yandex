/*
 * Completes an Inflight upload, permanently removing previously uploaded parts.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket.
 * - i_object_name:
 *     Name of the inflight Object to complete.
 * - i_object_created:
 *     Creation date of the inflight upload.
 * - i_inflight_created:
 *     Creation date of the inflight upload.
 *
 * Returns:
 *   A set of ``v1_code.inflight_result``.
 *
 * NOTE: On successful completion all object inflights's parts will be
 * permanently removed from ``s3.inflights`` table.
 *
 */
CREATE OR REPLACE FUNCTION v1_impl.complete_inflight_uploads(
    i_bid uuid,
    i_object_name text,
    i_object_created timestamp with time zone,
    i_inflight_created timestamp with time zone
) RETURNS SETOF v1_code.inflight_result
LANGUAGE plpgsql AS $$
DECLARE
    v_root_inflight v1_code.inflight_result;
    v_cid bigint;
BEGIN
    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in inflight's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_object_name);

    SELECT bid, name, object_created, inflight_created, part_id,
           mds_couple_id, mds_key_version, mds_key_uuid
        INTO v_root_inflight
        FROM s3.inflights
    WHERE bid = i_bid
        AND name = i_object_name
        AND object_created = i_object_created
        AND inflight_created = i_inflight_created
        AND part_id = 0
    FOR UPDATE;

    RETURN QUERY DELETE FROM s3.inflights
      WHERE bid = i_bid
        AND name = i_object_name
        AND object_created = i_object_created
        AND inflight_created = i_inflight_created
    RETURNING bid, name, object_created, inflight_created, part_id,
            mds_couple_id, mds_key_version, mds_key_uuid,
            metadata;
END;
$$;
