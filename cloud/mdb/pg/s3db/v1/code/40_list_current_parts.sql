/*
 * Returns some or all (up to ``i_limit``) object parts.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket list object parts from.
 * - i_name:
 *     Name of the multipart upload.
 * - i_created:
 *     Creation date of the multipart upload.
 * - i_start_part:
 *     Specifies the part_id to start listing from.
 * - i_limit:
 *     Sets the maximum number of object parts returned in the response.
 *
 * Returns:
 *   List of the v1_code.object_part that satisfy the search criteria
 *   specified by ``i_limit``.
 */
CREATE OR REPLACE FUNCTION v1_code.list_current_parts(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone,
    i_start_part integer DEFAULT 1,
    i_limit integer DEFAULT 10
) RETURNS SETOF v1_code.object_part
LANGUAGE sql STABLE AS $function$
    SELECT bid, cid, name, object_created, part_id, created, data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, metadata
        FROM s3.object_parts
        WHERE bid = i_bid
            AND name = i_name
            AND object_created = i_created
            AND part_id >= i_start_part
        ORDER BY part_id
        LIMIT i_limit;
$function$;
