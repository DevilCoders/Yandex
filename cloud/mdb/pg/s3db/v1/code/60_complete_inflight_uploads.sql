/*
 * Completes an Inflight upload (i.e. data migration), updating object and parts
 * and permanently removing previously uploaded parts.
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
 * - i_object_migrated:
 *     Deprecated object_migrated flag.
 * - i_separate_parts:
 *     Store parts in `completed_parts` table instead of the inline `parts` array.
 *
 * Returns:
 *   Nothing! (empty result set for return type compatibility with older versions)
 *
 * Raises:
 * - S3M01 (NoSuchUpload):
 *    If the requested inflight upload doesn't exist.
 *
 * NOTE: On successful completion all object inflights's parts will be
 * permanently removed from ``s3.inflights`` table.
 *
 */
CREATE OR REPLACE FUNCTION v1_code.complete_inflight_uploads(
    i_bid uuid,
    i_object_name text,
    i_object_created timestamp with time zone,
    i_inflight_created timestamp with time zone,
    i_object_migrated boolean DEFAULT NULL,
    i_separate_parts boolean DEFAULT FALSE
) RETURNS SETOF s3.object_part
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_object v1_code.object;
    v_updated_object v1_code.object;
    v_root_inflight v1_code.inflight_result;
    v_inflights v1_code.inflight_result[];
    v_inflights_metadata jsonb;
    v_null_parts_metadata jsonb;
    v_simple_object_part s3.object_part;
    v_inflight_object_parts s3.object_part[];
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

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such inflight upload'
            USING ERRCODE = 'S3M01';
    END IF;

    -- Gather object's inflights before acquiring object lock.
    SELECT array_agg((bid, name, object_created, inflight_created, part_id,
           mds_couple_id, mds_key_version, mds_key_uuid, metadata)::v1_code.inflight_result),
           jsonb_agg(a.metadata -> 'encryption' -> 'meta')
    INTO v_inflights, v_inflights_metadata
    FROM
    (
        SELECT *
        FROM s3.inflights
        WHERE bid = i_bid
            AND name = i_object_name
            AND object_created = i_object_created
            AND inflight_created = i_inflight_created
            AND part_id > 0
        ORDER BY part_id
    ) a;

    PERFORM v1_impl.lock_object(i_bid, i_object_name);

    -- Current object's version.
    EXECUTE format($quote$
        SELECT * FROM (
            SELECT bid, name, created, %4$L /* v_cid */, data_size, data_md5, mds_namespace, mds_couple_id,
                mds_key_version, mds_key_uuid, null_version, delete_marker,
                parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
            FROM s3.objects
            WHERE bid = %1$L /* i_bid */
            AND name = %2$L /* i_object_name */
            AND %3$L::timestamptz /* i_created */ = created
            UNION ALL
            SELECT bid, name, created, %4$L /* v_cid */, data_size, data_md5, NULL as mds_namespace, mds_couple_id,
                mds_key_version, mds_key_uuid, null_version, delete_marker,
                parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
            FROM s3.objects_noncurrent
            WHERE bid = %1$L /* i_bid */
            AND name = %2$L /* i_object_name */
            AND %3$L::timestamptz /* i_created */ = created
        ) AS a LIMIT 1
    $quote$, i_bid, i_object_name, i_object_created, v_cid) INTO v_object;

    IF
        v_object IS NULL /* no such object */ OR
        coalesce(v_object.parts_count, 0) != array_length(v_inflights, 1) /* part count mismatch */
    THEN
        PERFORM v1_code.abort_inflight_uploads(i_bid, i_object_name, i_object_created, i_inflight_created);

        RETURN;
    END IF;

    -- Move old object parts to storage delete queue.
    IF v_object.bid IS NOT NULL AND v_object.parts_count > 0 AND v_object.parts IS NULL THEN
        EXECUTE format($quote$
            SELECT v1_impl.storage_delete_queue_push((
                %1$L /* i_bid */, %4$L /* v_cid */, %2$L /* i_object_name */, %3$L /* i_created */,
                part_id, created, data_size, data_md5, NULL /* mds_namespace */,
                mds_couple_id, mds_key_version, mds_key_uuid, %5$L /* v_object.storage_class */,
                NULL /* i_metadata */
            )::v1_code.object_part)
            FROM s3.completed_parts
            WHERE bid = %1$L /* i_bid */
                AND name = %2$L /* i_object_name */
                AND %3$L::timestamptz /* i_created */ = object_created
        $quote$, i_bid, i_object_name, i_object_created, v_cid, v_object.storage_class);
    END IF;

    IF NOT i_separate_parts AND v_object.parts IS NOT NULL THEN
        -- Update an object stored in old format in place

        -- Prepare inflights as object_parts.
        SELECT array_agg((
            origin.part_id, i_object_created, origin.data_size, origin.data_md5,
            inflight.mds_couple_id, inflight.mds_key_version, inflight.mds_key_uuid
        )::s3.object_part)
        FROM unnest(coalesce(v_object.parts, array[]::s3.object_part[])) AS origin
        JOIN unnest(v_inflights) AS inflight
        ON (origin.part_id = inflight.part_id)
        INTO v_inflight_object_parts;

        -- NOTE: 'null'::jsonb is not null, so we need to compare it with equal sign
        SELECT jsonb_agg(elem) FROM jsonb_array_elements(v_inflights_metadata) AS elem WHERE elem = 'null'::jsonb INTO v_null_parts_metadata;

        -- Set `v_inflights_metadata` to null in case it is empty sequence or consist of nulls.
        IF jsonb_array_length(v_null_parts_metadata) IS NOT DISTINCT FROM
            jsonb_array_length(v_inflights_metadata) THEN
            v_inflights_metadata := NULL;
        END IF;

        -- Setup correct path for `v_inflights_metadata`.
        IF v_inflights_metadata IS NOT NULL THEN
            v_inflights_metadata := jsonb_set(coalesce(v_root_inflight.metadata, '{}'::jsonb),
            '{encryption, parts_meta}',
            v_inflights_metadata);
        END IF;

        -- Update object parts from prepared inflights.
        EXECUTE format($quote$
            WITH current AS (
                UPDATE s3.objects SET
                    mds_couple_id = %4$L, /* v_root_inflight.mds_couple_id */
                    mds_key_version = %5$L, /* v_root_inflight.mds_key_version */
                    mds_key_uuid = %6$L, /* v_root_inflight.mds_key_uuid */
                    parts = %7$L, /* v_inflight_object_parts */
                    metadata = coalesce(metadata - 'encryption', '{}'::jsonb) || (
                        CASE
                            WHEN %8$L IS NOT NULL THEN %8$L /* v_inflights_metadata */
                            WHEN %9$L IS NOT NULL THEN %9$L /* v_root_inflight.metadata */
                            ELSE '{}'::jsonb
                        END
                    )
                WHERE bid = %1$L /* i_bid */
                    AND name = %2$L /* i_object_name */
                    AND created = %3$L /* i_object_created */
                RETURNING (bid, name, created, NULL /* cid */, data_size, data_md5, mds_namespace,
                        mds_couple_id, mds_key_version, mds_key_uuid, null_version, delete_marker,
                        parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings)::v1_code.object AS updated
            ), noncurrent AS (
                UPDATE s3.objects_noncurrent SET
                    mds_couple_id = %4$L, /* v_root_inflight.mds_couple_id */
                    mds_key_version = %5$L, /* v_root_inflight.mds_key_version */
                    mds_key_uuid = %6$L, /* v_root_inflight.mds_key_uuid */
                    parts = %7$L, /* v_inflight_object_parts */
                    metadata = coalesce(metadata - 'encryption', '{}'::jsonb) || (
                        CASE
                            WHEN %8$L IS NOT NULL THEN %8$L /* v_inflights_metadata */
                            WHEN %9$L IS NOT NULL THEN %9$L /* v_root_inflight.metadata */
                            ELSE '{}'::jsonb
                        END
                    )
                WHERE bid = %1$L /* i_bid */
                    AND name =  %2$L /* i_object_name */
                    AND created = %3$L /* i_object_created */
                RETURNING (bid, name, created, NULL /* cid */, data_size, data_md5, NULL /* mds_namespace */,
                        mds_couple_id, mds_key_version, mds_key_uuid, null_version, delete_marker,
                        parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings)::v1_code.object AS updated
            ) SELECT (updated).*
                FROM (
                    SELECT updated FROM current
                    UNION ALL
                    SELECT updated FROM noncurrent
                ) AS a
        $quote$,
            /*  1 */ i_bid,
            /*  2 */ i_object_name,
            /*  3 */ i_object_created,
            /*  4 */ v_root_inflight.mds_couple_id,
            /*  5 */ v_root_inflight.mds_key_version,
            /*  6 */ v_root_inflight.mds_key_uuid,
            /*  7 */ v_inflight_object_parts,
            /*  8 */ v_inflights_metadata,
            /*  9 */ v_root_inflight.metadata
        ) INTO v_updated_object;

    ELSE

        IF v_object.parts IS NULL THEN
            -- Update object part data in place.
            EXECUTE format($quote$
                UPDATE s3.completed_parts o
                SET mds_couple_id=p.mds_couple_id, mds_key_version=p.mds_key_version,
                    mds_key_uuid=p.mds_key_uuid, encryption=p.metadata->'encryption'->>'meta'
                FROM (
                    SELECT (unnest(%1$L :: v1_code.inflight_result[] /* v_inflights */)).*
                ) p
                WHERE o.bid=%2$L AND o.name=%3$L AND o.object_created=%4$L AND p.part_id=o.part_id
            $quote$, v_inflights, i_bid, i_object_name, i_object_created);
        ELSE
            -- Object in the old format. Move object parts to completed_parts.
            EXECUTE format($quote$
                INSERT INTO s3.completed_parts (bid, name, object_created, part_id, end_offset,
                    created, data_size, data_md5, mds_couple_id, mds_key_version,
                    mds_key_uuid, storage_class, encryption)
                SELECT %3$L /* i_bid */, %4$L /* i_name */, %5$L /* i_object_created */,
                    op.part_id, (SUM(op.data_size) OVER (ORDER BY op.part_id)),
                    -- op.created may be NULL for old YDB objects
                    COALESCE(op.created, %5$L /* i_object_created */), op.data_size, op.data_md5,
                    np.mds_couple_id, np.mds_key_version, np.mds_key_uuid,
                    %6$L /* v_object.storage_class */, np.metadata->'encryption'->>'meta'
                FROM (SELECT (unnest(%1$L :: s3.object_part[] /* v_object.parts */)).*) op,
                    (SELECT (unnest(%2$L :: v1_code.inflight_result[] /* v_inflights */)).*) np
                WHERE np.part_id=op.part_id
            $quote$, v_object.parts, v_inflights, i_bid, i_object_name, i_object_created, v_object.storage_class);
        END IF;

        -- Update object row from root inflight.
        EXECUTE format($quote$
            WITH current AS (
                UPDATE s3.objects SET
                    mds_couple_id = %4$L, /* v_root_inflight.mds_couple_id */
                    mds_key_version = %5$L, /* v_root_inflight.mds_key_version */
                    mds_key_uuid = %6$L, /* v_root_inflight.mds_key_uuid */
                    parts = NULL,
                    metadata = coalesce(metadata - 'encryption', '{}'::jsonb) ||
                        coalesce(%7$L, '{}'::jsonb) /* v_root_inflight.metadata */
                WHERE bid = %1$L /* i_bid */
                    AND name = %2$L /* i_object_name */
                    AND created = %3$L /* i_object_created */
                RETURNING (bid, name, created, NULL /* cid */, data_size, data_md5, mds_namespace,
                        mds_couple_id, mds_key_version, mds_key_uuid, null_version, delete_marker,
                        parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings)::v1_code.object AS updated
            ), noncurrent AS (
                UPDATE s3.objects_noncurrent SET
                    mds_couple_id = %4$L, /* v_root_inflight.mds_couple_id */
                    mds_key_version = %5$L, /* v_root_inflight.mds_key_version */
                    mds_key_uuid = %6$L, /* v_root_inflight.mds_key_uuid */
                    parts = NULL,
                    metadata = coalesce(metadata - 'encryption', '{}'::jsonb) ||
                        coalesce(%7$L, '{}'::jsonb) /* v_root_inflight.metadata */
                WHERE bid = %1$L /* i_bid */
                    AND name = %2$L /* i_object_name */
                    AND created = %3$L /* i_object_created */
                RETURNING (bid, name, created, NULL /* cid */, data_size, data_md5, NULL /* mds_namespace */,
                        mds_couple_id, mds_key_version, mds_key_uuid, null_version, delete_marker,
                        parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings)::v1_code.object AS updated
            ) SELECT (updated).*
                FROM (
                    SELECT updated FROM current
                    UNION ALL
                    SELECT updated FROM noncurrent
                ) AS a
        $quote$,
            /* 1 */ i_bid,
            /* 2 */ i_object_name,
            /* 3 */ i_object_created,
            /* 4 */ v_root_inflight.mds_couple_id,
            /* 5 */ v_root_inflight.mds_key_version,
            /* 6 */ v_root_inflight.mds_key_uuid,
            /* 7 */ v_root_inflight.metadata
        ) INTO v_updated_object;

    END IF;

    v_simple_object_part := (
        NULL /* part id */,
        v_object.created,
        v_object.data_size,
        v_object.data_md5,
        v_object.mds_couple_id,
        v_object.mds_key_version,
        v_object.mds_key_uuid
    )::s3.object_part;

    -- Move old in-place object parts to storage delete queue.
    PERFORM v1_impl.storage_delete_queue_push((
        i_bid, v_cid, i_object_name, i_object_created, part.part_id /* part_id */,
        current_timestamp /* created */, 0 /* data_size */, part.data_md5,
        NULL /* mds_namespace */, part.mds_couple_id, part.mds_key_version, part.mds_key_uuid,
        NULL /* storage class */, NULL /* metadata */
    )::v1_code.object_part)
    FROM unnest(v_simple_object_part || coalesce(v_object.parts, array[]::s3.object_part[])) AS part
    WHERE mds_couple_id IS NOT NULL
        AND mds_key_version IS NOT NULL
        AND mds_key_uuid IS NOT NULL;

    PERFORM v1_impl.complete_inflight_uploads(i_bid, i_object_name, i_object_created, i_inflight_created);

    RETURN;

END;
$$;
