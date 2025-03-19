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
 * - i_range_start, i_range_end:
 *     Only return parts in the specified range (0/0 = return all parts; 0/-1 = do not return parts)
 *
 * Returns:
 *   An instance of ``code.object`` that represents the object's version, or
 *   empty set of ``code.object`` if there's no such object's version.
 */
CREATE OR REPLACE FUNCTION v1_code.object_info_with_parts(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone DEFAULT NULL,
    i_null_version boolean DEFAULT false,
    i_range_start bigint default 0,
    i_range_end bigint default 0
) RETURNS SETOF v1_code.object_with_part
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
        RETURN QUERY SELECT
            result.bid,
            result.name,
            result.created,
            NULL::timestamptz /* noncurrent */,
            result.data_size,
            result.data_md5,
            result.mds_couple_id,
            result.mds_key_version,
            result.mds_key_uuid,
            result.null_version,
            result.delete_marker,
            result.parts_count,
            result.storage_class,
            result.creator_id,
            (result.metadata #- '{encryption,parts_meta}') metadata,
            result.acl,
            result.lock_settings,
            (NULL::v1_code.composite_part_info).*;

        IF result.parts_count > 0 THEN
            IF i_range_start < 0 THEN
                i_range_start := result.data_size + i_range_start;
            END IF;
            IF i_range_end = 0 THEN
                i_range_end := result.data_size;
            END IF;
            IF result.parts IS NULL THEN
                RETURN QUERY EXECUTE format($quote$
                    SELECT
                        (NULL::v1_code.composite_object_info).*,
                        o.part_id,
                        o.end_offset part_end_offset,
                        o.created part_created,
                        o.data_size part_data_size,
                        o.data_md5 part_data_md5,
                        o.mds_couple_id part_mds_couple_id,
                        o.mds_key_version part_mds_key_version,
                        o.mds_key_uuid part_mds_key_uuid,
                        o.storage_class part_storage_class,
                        o.encryption part_encryption
                    FROM s3.completed_parts o
                    WHERE o.bid=%1$L AND o.name=%2$L AND o.object_created=%3$L
                    AND o.end_offset > %4$L
                    AND o.end_offset-o.data_size < %5$L
                $quote$, result.bid, result.name, result.created, i_range_start, i_range_end);
            ELSE
                RETURN QUERY SELECT
                    (NULL::v1_code.composite_object_info).*,
                    o.*
                FROM (
                    SELECT
                        o.part_id,
                        (SUM(o.data_size) OVER (ORDER BY o.part_id))::bigint part_end_offset,
                        o.created part_created,
                        o.data_size part_data_size,
                        o.data_md5 part_data_md5,
                        o.mds_couple_id part_mds_couple_id,
                        o.mds_key_version part_mds_key_version,
                        o.mds_key_uuid part_mds_key_uuid,
                        result.storage_class part_storage_class,
                        e.encryption part_encryption
                    FROM (SELECT (unnest(result.parts)).*) o
                    LEFT JOIN jsonb_array_elements_text(result.metadata->'encryption'->'parts_meta')
                        WITH ORDINALITY AS e (encryption, part_id)
                        ON e.part_id=o.part_id
                ) o
                WHERE o.part_end_offset > i_range_start
                    AND o.part_end_offset-o.part_data_size < i_range_end;
            END IF;
        END IF;
    END IF;
END;
$$;
