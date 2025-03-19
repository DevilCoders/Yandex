/*
 * Returns Multipart Upload.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket list object parts from.
 * - i_name:
 *     Name of the multipart upload.
 * - i_created:
 *     Creation date of the multipart upload.
 *
 * Returns:
 *   "Root" multipart upload's record -- an instance of ``code.multipart_upload``
 *   with ``part_id`` of 0.
 */
CREATE OR REPLACE FUNCTION v1_code.multipart_upload_info(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone
) RETURNS SETOF v1_code.multipart_upload
LANGUAGE sql STABLE AS $function$
    SELECT bid, cid, name, object_created, part_id, created, 0::bigint AS data_size, data_md5,
            mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class,
            creator_id, metadata, acl, lock_settings
        FROM s3.object_parts
        WHERE bid = i_bid
            AND name = i_name
            AND object_created = i_created
            AND part_id = 0;
$function$;
