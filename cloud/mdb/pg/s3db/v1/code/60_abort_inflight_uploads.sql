/*
 * Aborts all object's inflight uploads.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket to abort inflight upload from.
 * - i_object_name:
 *     Name of the inflight object to abort.
 * - i_object_created:
 *     Creation date of the infligt's upload object.
 * - i_inflight_created:
 *     timestamp of infligt created.
 *
 * Returns:
 *   Count of deleted inflights.
 *
 */
CREATE OR REPLACE FUNCTION v1_code.abort_inflight_uploads(
    i_bid uuid,
    i_object_name text,
    i_object_created timestamp with time zone,
    i_inflight_created timestamp with time zone
) RETURNS SETOF v1_code.inflight_result
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_root_inflight v1_code.inflight_result;
BEGIN
    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in object_parts's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_object_name);

    SELECT bid, name, object_created, inflight_created, part_id,
           mds_couple_id, mds_key_version, mds_key_uuid, metadata
        INTO v_root_inflight
        FROM s3.inflights
        WHERE bid = i_bid
            AND name = i_object_name
            AND object_created = i_object_created
            AND inflight_created = i_inflight_created
            AND part_id = 0
        FOR UPDATE;

    RETURN QUERY WITH deleted_inflights AS (
        DELETE FROM s3.inflights
            WHERE bid = i_bid
                AND name = i_object_name
                AND object_created = i_object_created
                AND inflight_created = i_inflight_created
        RETURNING bid, name, object_created, inflight_created, part_id,
            mds_couple_id, mds_key_version, mds_key_uuid,
            metadata
    ),
    delete_queue AS (
        SELECT part_id, v1_impl.storage_delete_queue_push_inflight(deleted_inflights)
        FROM deleted_inflights
        WHERE mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL
            AND mds_key_uuid IS NOT NULL
    )
    SELECT bid, name, object_created, inflight_created, dq.part_id,
        mds_couple_id, mds_key_version, mds_key_uuid,
        metadata FROM deleted_inflights di JOIN delete_queue dq ON di.part_id = dq.part_id;

END;
$$;
