/*
 * Deletes multiple bucket's objects versions.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket to delete the objects from.
 * - i_multiple_drop_objects:
 *     Element of multiple dropping, that contains key name and creation date (or NULL for NULL version).
 *
 * Returns:
 * - Deleted object's version or delete marker or error.
 */
CREATE OR REPLACE FUNCTION v1_code.drop_multiple_versions(
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_multiple_drop_objects v1_code.multiple_drop_object[]
) RETURNS SETOF v1_code.multiple_drop_object_result
LANGUAGE plpgsql AS $$
DECLARE
    v_deleted_version v1_code.object_version;
    v_multiple_drop_object v1_code.multiple_drop_object;
BEGIN
    RETURN QUERY EXECUTE format($quote$
        WITH keys AS (
            SELECT name, created, v1_impl.lock_object(%1$L /* i_bid */, name)
            FROM unnest(%2$L::v1_code.multiple_drop_object[] /* i_multiple_drop_objects */)
            GROUP BY name, created
        ), names AS (
            SELECT name FROM keys
            GROUP BY name
        ), drop_object as (
            DELETE FROM s3.objects o
                USING keys as k
            WHERE o.bid =  %1$L /* i_bid */
                AND o.name = k.name
                AND (
                    k.created IS NULL AND o.null_version
                    OR
                    k.created = o.created AND NOT o.null_version
                )
            RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), o.data_size,
                o.data_md5, o.mds_namespace, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                o.null_version, false  /*delete_marker*/, o.parts_count, o.parts, o.storage_class, o.creator_id,
                o.metadata, o.acl, o.lock_settings)::v1_code.object as removed_object
        ), drop_del_mark as (
            DELETE FROM s3.object_delete_markers o
                USING keys as k
            WHERE o.bid =  %1$L /* i_bid */
                AND o.name = k.name
                AND (
                    k.created IS NULL AND o.null_version
                    OR
                    k.created = o.created AND NOT o.null_version
                )
            RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), NULL /*data_size*/,
                NULL /*data_md5*/, NULL /*mds_namespace*/, NULL /*mds_couple_id*/,
                NULL /*mds_key_version*/, NULL /*mds_key_uuid*/, o.null_version,
                true /*delete_marker*/, NULL /*parts_count*/, NULL /*parts*/, NULL /*storage_class*/,
                o.creator_id, NULL /*metadata*/, NULL /*acl*/, NULL /*lock_settings*/)::v1_code.object as removed_object
        ), drop_noncurrent as (
            DELETE FROM s3.objects_noncurrent o
                USING keys as k
            WHERE o.bid =  %1$L /* i_bid */
                AND o.name = k.name
                AND (
                    k.created IS NULL AND o.null_version
                    OR
                    k.created = o.created AND NOT o.null_version
                )
            RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), o.data_size,
                o.data_md5, NULL /*mds_namespace*/, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                o.null_version, o.delete_marker, o.parts_count, o.parts, o.storage_class, o.creator_id,
                o.metadata, o.acl, o.lock_settings)::v1_code.object as removed_object, o.noncurrent
        ), queues as (
            SELECT
                v1_impl.storage_delete_queue_push(removed_object),
                v1_impl.drop_object_parts(removed_object),
                v1_code.chunks_counters_queue_push(
                    OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(removed_object)),
                (removed_object).name as name,
                (removed_object).created as created,
                (removed_object).null_version as null_version,
                (removed_object).delete_marker as delete_marker,
                (removed_object).cid as cid,
                noncurrent
            FROM (
                SELECT removed_object, noncurrent FROM drop_noncurrent
                UNION ALL
                SELECT removed_object, NULL FROM drop_object
                UNIon ALL
                SELECT removed_object, NULL FROM drop_del_mark
            ) as a
        ), prev_versions as (
            SELECT DISTINCT ON (o.name) o.name, o.created
            FROM s3.objects_noncurrent o
                INNER JOIN queues q ON q.noncurrent IS NULL
                    AND o.name = q.name
                LEFT JOIN drop_noncurrent d ON o.name = (d.removed_object).name
                    AND o.created = (d.removed_object).created
            WHERE o.bid = %1$L /* i_bid */
                AND d.removed_object IS NULL
            ORDER BY o.name, o.created desc
        ), versions_to_restore as (
            DELETE FROM s3.objects_noncurrent o
                USING prev_versions as k
            WHERE o.bid =  %1$L /* i_bid */
                AND o.name = k.name
                AND o.created = k.created
            RETURNING o.bid, o.name, o.created, o.data_size, o.data_md5, o.mds_couple_id, o.mds_key_version,
                mds_key_uuid, o.null_version, o.delete_marker, o.parts_count, o.parts, o.storage_class,
                creator_id, o.metadata, o.acl, o.lock_settings
        ), object_insert as (
            INSERT INTO s3.objects (
                bid, name, created, data_size, data_md5, mds_couple_id, mds_key_version,
                mds_key_uuid, null_version, delete_marker, parts_count, parts, storage_class,
                creator_id, metadata, acl, lock_settings)
            SELECT bid, name, created, data_size, data_md5, mds_couple_id, mds_key_version,
                mds_key_uuid, null_version, delete_marker, parts_count, parts, storage_class,
                creator_id, metadata, acl, lock_settings
            FROM versions_to_restore
            WHERE NOT delete_marker
        ), del_mark_insert as (
            INSERT INTO s3.object_delete_markers (bid, name, created, creator_id, null_version)
            SELECT bid, name, created, creator_id, null_version
            FROM versions_to_restore
            WHERE delete_marker
        )
        SELECT k.name, k.created, q.delete_marker,
            NULL::timestamp with time zone as delete_marker_created,
            CASE WHEN (q.name) IS NULL THEN 'S3K01' ELSE NULL::text END AS error
        FROM keys k
            LEFT JOIN queues as q ON k.name = q.name
                AND (
                        k.created IS NULL AND q.null_version
                        OR
                        k.created = q.created AND NOT q.null_version
                    )
    $quote$, i_bid, i_multiple_drop_objects);
END;
$$;
