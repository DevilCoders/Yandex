/*
 * Initiates a Inflight Upload.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket.
 * - i_object_name:
 *     Name of the inflight upload to initiate.
 * - i_object_created:
 *     Object creation timestamp.
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
 *   An instance of ``v1_code.inflight_result`` that represents added inflight upload.
 */
CREATE OR REPLACE FUNCTION v1_code.start_inflight_upload(
    i_bid uuid,
    i_object_name text,
    i_object_created timestamp with time zone,
    i_mds_couple_id integer DEFAULT NULL,
    i_mds_key_version integer DEFAULT NULL,
    i_mds_key_uuid uuid DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL
) RETURNS v1_code.inflight_result
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_res v1_code.inflight_result;
BEGIN
    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in object_parts's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_object_name);

    INSERT INTO s3.inflights (
        bid, name, object_created, inflight_created, part_id,
        mds_couple_id, mds_key_version, mds_key_uuid,
        metadata
    ) VALUES (
        i_bid, i_object_name, i_object_created, current_timestamp, 0,
        i_mds_couple_id, i_mds_key_version, i_mds_key_uuid,
        i_metadata
    ) RETURNING bid, name, object_created, inflight_created, part_id,
        mds_couple_id, mds_key_version, mds_key_uuid,
        metadata
    INTO v_res;

    RETURN v_res;
END;
$$;
