/*
 * Adds a part to inflights uploads.
 * - i_bid:
 *     ID of the bucket.
 * - i_object_name:
 *     Name of the inflight upload to initiate.
 * - i_object_created:
 *     Object created timestamp.
 * - i_mds_couple_id:
 *     Couple ID part of MDS key.
 * - i_mds_key_version:
 *     MDS key version part of MDS key.
 * - i_mds_key_uuid:
 *     MDS key UUID part of MDS key.
 * - i_metadata:
 *     Reserved for part metadata.
 *
 * Returns:
 *   An instance of ``v1_code.inflight_result`` that represents added inflight upload,
 *   or empty set if such inflight upload wasn't started.
 *
 */
CREATE OR REPLACE FUNCTION v1_code.add_inflight_upload(
    i_bid uuid,
    i_object_name text,
    i_object_created timestamp with time zone,
    i_inflight_created timestamp with time zone,
    i_part_id integer,
    i_mds_couple_id integer DEFAULT NULL,
    i_mds_key_version integer DEFAULT NULL,
    i_mds_key_uuid uuid DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL
) RETURNS SETOF v1_code.inflight_result
LANGUAGE plpgsql AS $$
DECLARE
    v_first_inflight v1_code.inflight_result;
    v_current_inflight v1_code.inflight_result;
    v_res v1_code.inflight_result;
    v_cid bigint;
BEGIN
    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in object_parts's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_object_name);

    SELECT bid, name, object_created, inflight_created, part_id,
           mds_couple_id, mds_key_version, mds_key_uuid
        INTO v_first_inflight
        FROM s3.inflights
    WHERE bid = i_bid
        AND name = i_object_name
        AND object_created = i_object_created
        AND inflight_created = i_inflight_created
        AND part_id = 0
      FOR NO KEY UPDATE;

    IF NOT FOUND THEN
        SELECT
            i_bid,
            i_object_name,
            i_object_created,
            i_inflight_created,
            i_part_id,
            i_mds_couple_id,
            i_mds_key_version,
            i_mds_key_uuid,
            i_metadata
        INTO v_current_inflight;

        PERFORM v1_impl.storage_delete_queue_push_inflight(v_current_inflight);
        RETURN;
    END IF;

    SELECT bid, name, object_created, inflight_created, part_id,
           mds_couple_id, mds_key_version, mds_key_uuid,
           metadata
    INTO v_current_inflight
    FROM s3.inflights
    WHERE bid = i_bid
          AND name = i_object_name
          AND object_created = i_object_created
          AND inflight_created = i_inflight_created
          AND part_id = i_part_id;

    IF FOUND THEN
        /*
         * NOTE: Due to connectivity issues client could receive error, while
         * transaction was actually committed, and retry the request with the same
         * arguments.
         *
         * Here, we check whether the current version of the inflight is the same as
         * requested and respond with the current version, if it is.
         */
        IF v_current_inflight.mds_couple_id IS NOT DISTINCT FROM i_mds_couple_id
            AND v_current_inflight.mds_key_version IS NOT DISTINCT FROM i_mds_key_version
            AND v_current_inflight.mds_key_uuid IS NOT DISTINCT FROM i_mds_key_uuid
            AND v_current_inflight.metadata IS NOT DISTINCT FROM i_metadata
        THEN
            -- Return current inflight `as is`.
            -- NOTE: original code was: `v_res := v_current_inflight`, but inplace update
            -- with current value is required for replication synchtonization.
            UPDATE s3.inflights
            SET name = name
            WHERE bid = v_current_inflight.bid
                AND name = v_current_inflight.object_name
                AND object_created = v_current_inflight.object_created
                AND inflight_created = v_current_inflight.inflight_created
                AND part_id = v_current_inflight.part_id
            RETURNING bid, name, object_created, inflight_created, part_id,
                mds_couple_id, mds_key_version, mds_key_uuid, metadata
            INTO v_res;
        ELSE
            -- Update Storage key for existing inflight.
            UPDATE s3.inflights
            SET mds_couple_id = i_mds_couple_id,
                mds_key_version = i_mds_key_version,
                mds_key_uuid = i_mds_key_uuid,
                metadata = i_metadata
            WHERE bid = i_bid
                AND name = i_object_name
                AND object_created = i_object_created
                AND inflight_created = i_inflight_created
                AND part_id = i_part_id
            RETURNING bid, name, object_created, inflight_created, part_id,
                mds_couple_id, mds_key_version, mds_key_uuid, metadata
            INTO v_res;

            PERFORM v1_impl.storage_delete_queue_push_inflight(v_current_inflight);
        END IF;
    ELSE
        -- Insert new inflight.
        INSERT INTO s3.inflights (
            bid, name, object_created, inflight_created, part_id,
            mds_couple_id, mds_key_version, mds_key_uuid,
            metadata
        ) VALUES (
            i_bid, i_object_name, i_object_created, i_inflight_created, i_part_id,
            i_mds_couple_id, i_mds_key_version, i_mds_key_uuid,
            i_metadata
        ) RETURNING bid, name, object_created, inflight_created, part_id,
            mds_couple_id, mds_key_version, mds_key_uuid,
            metadata
        INTO v_res;
    END IF;

    RETURN NEXT v_res;
END;
$$;
