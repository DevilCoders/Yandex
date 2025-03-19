/*
 * Returns the bucket object or its current version.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket to get the object from.
 * - i_name:
 *     Name of the object to get.
 * - i_created, i_null_version:
 *     Arguments for backward compatibility
 *
 * Returns:
 *   An instance of ``code.object`` that represents the object's version, or
 *   empty set of ``code.object`` if there's no such object's version.
 */
CREATE OR REPLACE FUNCTION v1_code.object_info(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone DEFAULT NULL,
    i_null_version boolean DEFAULT false
) RETURNS SETOF v1_code.object
LANGUAGE plpgsql AS $$
DECLARE
    result v1_code.object;
BEGIN
    IF i_created IS NOT NULL OR coalesce(i_null_version, false) THEN
        RAISE EXCEPTION 'Only current version of the object could be requested'
            -- NOTE: 0A000 code is "feature_not_supported"
            USING ERRCODE = '0A000';
    END IF;

    EXECUTE format($quote$
        SELECT *
        FROM (
            SELECT bid, name, created, cid, data_size, data_md5, mds_namespace,
                    mds_couple_id, mds_key_version, mds_key_uuid,
                    null_version, delete_marker, parts_count, parts, storage_class,
                    creator_id, metadata, acl, lock_settings
                FROM s3.objects
                WHERE bid = %1$L /* i_bid */
                    AND name = %2$L /* i_name */
            UNION ALL
            SELECT bid, name, created, NULL AS cid, NULL AS data_size,
                    NULL AS data_md5, NULL AS mds_namespace, NULL AS mds_couple_id,
                    NULL AS mds_key_version, NULL AS mds_key_uuid, null_version,
                    true AS delete_marker, NULL AS parts_count, NULL AS parts, NULL AS storage_class,
                    NULL AS creator_id, NULL AS metadata, NULL AS acl, NULL AS lock_settings
                FROM s3.object_delete_markers
                WHERE bid = %1$L /* i_bid */
                    AND name = %2$L /* i_name */
        ) as a
        LIMIT 1
    $quote$, i_bid, i_name) INTO result;

    IF result IS NULL THEN
        -- Check whether object chunk is still present on this shard and raise S3X01 if not
        PERFORM v1_code.get_object_cid_non_blocking(i_bid, i_name, TRUE);
    ELSE
        -- Be compatible with older code that uses the unnest_object_parts() wrapper
        IF result.parts_count IS NOT NULL AND result.parts IS NULL THEN
            EXECUTE format($quote$
                SELECT
                    array_agg((
                        o.part_id, o.created, o.data_size, o.data_md5,
                        o.mds_couple_id, o.mds_key_version, o.mds_key_uuid
                    )::s3.object_part order by o.end_offset),
                    case when count(o.encryption)=0 then %4$L::jsonb else jsonb_set(
                        ('{"encryption":{}}'::jsonb || coalesce(%4$L::jsonb, '{}'::jsonb)),
                        '{encryption,parts_meta}',
                        jsonb_agg(o.encryption order by o.end_offset)
                    ) end
                FROM s3.completed_parts o
                WHERE o.bid=%1$L AND o.name=%2$L AND o.object_created=%3$L
            $quote$, result.bid, result.name, result.created, result.metadata)
            INTO result.parts, result.metadata;
        END IF;
        RETURN NEXT result;
    END IF;
END;
$$;
