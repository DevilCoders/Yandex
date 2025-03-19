/*
 * Adds new object delete marker to the bucket.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket to add the object to.
 * - i_name:
 *     Name of the object to add.
 * - i_versioning:
 *     Versioning state of the bucket.
 * - i_created:
 *     Version creation date or NULL for null version.
 * - i_creator_id:
 *     Requester ID.
 *
 * Returns:
 *   An instance of ``v1_code.object_delete_marker`` that represents added object delete marker.
 *
 * NOTE: If the bucket's versioning isn't 'enabled' previous 'null' version of
 * the object will be removed, if any.
 */
CREATE OR REPLACE FUNCTION v1_code.add_delete_marker(
    i_bid uuid,
    i_name text,
    i_versioning s3.bucket_versioning_type,
    i_created timestamp with time zone DEFAULT NULL,
    i_creator_id text DEFAULT NULL
) RETURNS SETOF v1_code.object_delete_marker
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_current_object v1_code.object;
    v_delete_marker v1_code.object_delete_marker;
    v_chunk_counters_change v1_code.chunk_counters;
BEGIN
    IF i_versioning = 'disabled' THEN
        RAISE EXCEPTION 'Disabled bucket versioning is not support delete markers'
            -- NOTE: 0A000 code is "feature_not_supported"
            USING ERRCODE = '0A000';
    END IF;

    /* Block chunk to share, return this chunk id as object's cid */
    v_cid := v1_code.get_object_cid(i_bid, i_name);
    PERFORM v1_impl.lock_object(i_bid, i_name);

    v_current_object := v1_impl.drop_object_current(i_bid, v_cid, i_name, i_created);

    IF i_created IS NOT NULL and v_current_object IS NULL THEN
        RETURN;
    END IF;

    IF NOT v_current_object IS NULL THEN
        PERFORM v1_impl.check_old_transaction(i_versioning, (v_current_object).created);

        IF i_versioning = 'enabled' THEN
            PERFORM v1_impl.push_object_noncurrent(v_current_object);
        ELSE
            IF v_current_object.null_version THEN
                -- Delete parts of the current version because we delete the version itself
                PERFORM v1_impl.drop_object_parts(v_current_object);
                PERFORM v1_impl.storage_delete_queue_push(v_current_object);
                PERFORM v1_code.chunks_counters_queue_push(
                    OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(v_current_object));
            ELSE
                -- Leave parts as is because we only move current version to noncurrent
                PERFORM v1_impl.remove_null_object_noncurrent(i_bid, v_cid, i_name);
                PERFORM v1_impl.push_object_noncurrent(v_current_object);
            END IF;
        END IF;
    END IF;

    EXECUTE format($quote$
        INSERT INTO s3.object_delete_markers (bid, name, null_version, creator_id)
        VALUES(
            %1$L /* i_bid */,
            %2$L /* i_name */,
            %3$L /* i_versioning */!='enabled',
            %4$L /* i_creator_id */
        )
        RETURNING bid, name, created, creator_id, null_version
    $quote$, i_bid, i_name, i_versioning, i_creator_id) INTO v_delete_marker;

    PERFORM v1_code.chunks_counters_queue_push(v1_code.delete_marker_get_chunk_counters(v_cid, v_delete_marker));

    RETURN NEXT v_delete_marker;
END;
$$;
