/*
 * Deletes bucket's object and all it's versions. Use only for disabled versioning
 *
 * Args:
 * - i_bid:
 *     ID of the bucket to delete the object from.
 * - i_name:
 *     Name of the object to delete.
 * - i_versioning:
 *     Versioning state of the bucket. Only 'disables' supported.
 * - i_created:
 *     Creation date of the object. If this is ``NULL`` current version of the
 *     object should be deleted, otherwise specific version of the object should
 *     be deleted.
 * - i_bucket_name:
 *     It is temporary parameter and it was added to this func to be consistent
 *     with plproxy function.
 *
 * Returns:
 * - Deleted object (or it's current version) if either the bucket's versioning state is
 *   'disabled' or a specific version of the object is requested.
 */
CREATE OR REPLACE FUNCTION v1_code.drop_object(
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_created timestamp with time zone DEFAULT NULL,
    i_bucket_name text DEFAULT NULL
) RETURNS SETOF v1_code.object
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_removed_object v1_code.object;
BEGIN
    -- Only 'disabled' versioning is supported in this function
    IF i_versioning != 'disabled' THEN
        RAISE EXCEPTION 'Only disabled bucket versioning is supported'
            -- NOTE: 0A000 code is "feature_not_supported"
            USING ERRCODE = '0A000';
    END IF;

    /* Block chunk to share, return this chunk id as object's cid */
    v_cid := v1_code.get_object_cid(i_bid, i_name);
    PERFORM v1_impl.lock_object(i_bid, i_name);

    /*
    * In case of 'disabled' versioning state only null versions of objects
    * are present in the bucket and each object has exactly one null version.
    * But in case of inconsistency we should drop all object versions by key.
    */
    EXECUTE format($quote$
        WITH drop_object as (
            DELETE FROM s3.objects
                WHERE bid =  %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND (%3$L is NULL OR created = %3$L) /* i_created */
            RETURNING (bid, name, created, %4$L /* v_cid */, data_size,
                data_md5, mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
                null_version, false  /*delete_marker*/, parts_count, parts, storage_class, creator_id,
                metadata, acl, lock_settings)::v1_code.object as removed_object, 0 as remove_order
        ), drop_del_mark as (
            DELETE FROM s3.object_delete_markers
                WHERE bid =  %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND (%3$L is NULL OR created = %3$L) /* i_created */
            RETURNING (bid, name, created, %4$L /* v_cid */, NULL /*data_size*/,
                NULL /*data_md5*/, NULL /*mds_namespace*/, NULL /*mds_couple_id*/,
                NULL /*mds_key_version*/, NULL /*mds_key_uuid*/, null_version,
                true /*delete_marker*/, NULL /*parts_count*/, NULL /*parts*/, NULL /*storage_class*/,
                creator_id, NULL /*metadata*/, NULL /*acl*/, NULL /*lock_settings*/)::v1_code.object as removed_object, 1 as remove_order
        ), drop_noncurrent as (
            DELETE FROM s3.objects_noncurrent
                WHERE bid =  %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND EXISTS(SELECT 1 from drop_object UNION SELECT 1 FROM drop_del_mark)
            RETURNING (bid, name, created, %4$L /* v_cid */, data_size,
                data_md5, NULL /*mds_namespace*/, mds_couple_id, mds_key_version, mds_key_uuid,
                null_version, delete_marker, parts_count, parts, storage_class, creator_id,
                metadata, acl, lock_settings)::v1_code.object as removed_object, 2 as remove_order
        ), queues as (
            SELECT
                removed_object, remove_order,
                v1_impl.storage_delete_queue_push(removed_object),
                v1_impl.drop_object_parts(removed_object),
                v1_code.chunks_counters_queue_push(
                    OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(removed_object))
            FROM (
                SELECT removed_object, remove_order FROM drop_object
                UNION ALL
                SELECT removed_object, remove_order FROM drop_del_mark
                UNION ALL
                SELECT removed_object, remove_order FROM drop_noncurrent
            ) as a
        )
        SELECT (o[1]).*
        FROM (
            SELECT array_agg(removed_object order by remove_order) as o
            FROM queues
        ) as a
    $quote$, i_bid, i_name, i_created, v_cid) INTO v_removed_object;

    IF NOT v_removed_object IS NULL THEN
        RETURN NEXT v_removed_object;
    END IF;
END;
$$;
