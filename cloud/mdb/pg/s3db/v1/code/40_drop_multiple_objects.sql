/*
 * Deletes multiple bucket's objects.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket to delete the objects from.
 * - i_versioning:
 *     Versioning state of the bucket.
 * - i_multiple_drop_objects:
 *     Element of multiple dropping, that contains key name and creation date.
 *     If creation date is ``NULL`` current version of the object should be deleted, otherwise
 *     specific version of the object should be deleted.
 * - i_bucket_name:
 *     It is temporary parameter and it was added to this func to be consistent
 *     with plproxy function.
 * - i_creator_id:
 *     Requester ID.
 *
 * TODO: Remove i_bucket_name parameter after refusing from plproxy
 *
 * Returns:
 * - Deleted object's version or delete marker or error.
 */
CREATE OR REPLACE FUNCTION v1_code.drop_multiple_objects(
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_multiple_drop_objects v1_code.multiple_drop_object[],
    i_bucket_name text DEFAULT NULL,
    i_creator_id text DEFAULT NULL
) RETURNS SETOF v1_code.multiple_drop_object_result
LANGUAGE plpgsql AS $$
BEGIN
    IF i_versioning != 'disabled' THEN
        -- lock objects before CTE starts. Lock inside CTE has problems because CTE sees old snapshot
        PERFORM v1_impl.lock_object(i_bid, name)
        FROM unnest(i_multiple_drop_objects);

        RETURN QUERY EXECUTE format($quote$
            WITH keys AS (
                SELECT name
                FROM unnest(%2$L::v1_code.multiple_drop_object[] /* i_multiple_drop_objects */)
                GROUP BY name
            ), drop_cur_object AS (
                DELETE FROM s3.objects o
                    USING keys AS k
                WHERE o.bid = %1$L /* i_bid */
                    AND o.name = k.name
                RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), o.data_size,
                    o.data_md5, o.mds_namespace, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                    o.null_version, false  /*delete_marker*/, o.parts_count, o.parts, o.storage_class, o.creator_id,
                    o.metadata, o.acl, o.lock_settings)::v1_code.object AS removed_object, o.name
            ), drop_del_mark AS (
                DELETE FROM s3.object_delete_markers o
                    USING keys AS k
                WHERE o.bid = %1$L /* i_bid */
                    AND o.name = k.name
                RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), NULL /*data_size*/,
                    NULL /*data_md5*/, NULL /*mds_namespace*/, NULL /*mds_couple_id*/,
                    NULL /*mds_key_version*/, NULL /*mds_key_uuid*/, o.null_version,
                    true /*delete_marker*/, NULL /*parts_count*/, NULL /*parts*/, NULL /*storage_class*/,
                    o.creator_id, NULL /*metadata*/, NULL /*acl*/, NULL /*lock_settings*/)::v1_code.object AS removed_object, o.name
            ), drop_object AS (
                SELECT removed_object, name
                FROM (
                    SELECT removed_object, name FROM drop_cur_object
                    UNION ALL
                    SELECT removed_object, name FROM drop_del_mark
                ) AS a
            ), collision_objects AS (
                SELECT name, v1_impl.check_old_transaction(%3$L /* i_versioning */, (removed_object).created)
                FROM drop_object
            ), add_del_mark AS (
                INSERT INTO s3.object_delete_markers (bid, name, creator_id, null_version)
                SELECT
                    %1$L /* i_bid */,
                    keys.name,
                    %4$L /* i_creator_id */,
                    %3$L /* i_versioning */!='enabled'::s3.bucket_versioning_type
                FROM keys LEFT JOIN drop_object ON keys.name = drop_object.name
                RETURNING (bid, name, created, creator_id, null_version)::v1_code.object_delete_marker AS marker, name
            ), queues AS (
                SELECT
                    (add_del_mark.marker),
                    v1_code.chunks_counters_queue_push(
                        v1_code.delete_marker_get_chunk_counters(
                            coalesce((removed_object).cid, v1_code.get_object_cid(%1$L /* i_bid */, (add_del_mark.marker).name)),
                            add_del_mark.marker
                        )
                    ),
                    CASE WHEN
                        drop_object.name IS NOT NULL AND %3$L/* i_versioning */='enabled'::s3.bucket_versioning_type
                        OR NOT (removed_object).null_version
                    THEN
                        v1_impl.push_object_noncurrent((removed_object))
                    END,
                    CASE WHEN
                        %3$L/* i_versioning */!='enabled'::s3.bucket_versioning_type
                        AND NOT (removed_object).null_version
                    THEN
                        v1_impl.remove_null_object_noncurrent(
                            (removed_object).bid,
                            (removed_object).cid,
                            (removed_object).name
                        )
                    END,
                    CASE WHEN
                        %3$L/* i_versioning */!='enabled'::s3.bucket_versioning_type
                        AND (removed_object).null_version
                    THEN
                        v1_impl.storage_delete_queue_push((removed_object))
                    END,
                    CASE WHEN
                        %3$L/* i_versioning */!='enabled'::s3.bucket_versioning_type
                        AND (removed_object).null_version
                    THEN
                        v1_code.chunks_counters_queue_push(
                            OPERATOR(v1_code.-) v1_code.object_get_chunk_counters((removed_object)))
                    END
                FROM add_del_mark
                    LEFT JOIN drop_object on add_del_mark.name = drop_object.name
                    LEFT JOIN collision_objects on add_del_mark.name = collision_objects.name
            )
            SELECT (marker).name, CASE WHEN (marker).null_version THEN NULL ELSE (marker).created END,
                true AS delete_marker, (marker).created, NULL::text AS error
            FROM queues;
        $quote$, i_bid, i_multiple_drop_objects, i_versioning, i_creator_id);
    ELSE
        RETURN QUERY EXECUTE format($quote$
            WITH keys AS (
                SELECT %1$L::uuid /* i_bid */ AS bid, name
                FROM unnest(%2$L::v1_code.multiple_drop_object[] /* i_multiple_drop_objects */)
                GROUP BY name
            ), objects AS (
                SELECT o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name) AS cid, o.data_size,
                    o.data_md5, o.mds_namespace, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                    o.null_version, false  /*delete_marker*/, o.parts_count, o.parts, o.storage_class, o.creator_id,
                    o.metadata, o.acl, o.lock_settings
                FROM s3.objects AS o
                JOIN keys AS k
                ON o.bid = %1$L /* i_bid */
                    AND o.bid = k.bid
                    AND o.name = k.name
            ), lock AS (
                SELECT name, v1_impl.lock_object(%1$L /* i_bid */, name)
                FROM objects
            ), drop_object AS (
                DELETE FROM s3.objects AS o
                    USING objects AS k
                    WHERE o.bid = %1$L /* i_bid */
                        AND o.bid = k.bid
                        AND o.name = k.name
                RETURNING (o.bid, o.name, o.created, k.cid, o.data_size,
                    o.data_md5, o.mds_namespace, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                    o.null_version, false  /*delete_marker*/, o.parts_count, o.parts, o.storage_class, o.creator_id,
                    o.metadata, o.acl, NULL::JSONB /* lock_settings */)::v1_code.object AS removed_object
            ), queues AS (
                SELECT removed_object,
                    v1_impl.storage_delete_queue_push(removed_object),
                    v1_impl.drop_object_parts(removed_object),
                    v1_code.chunks_counters_queue_push(
                            OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(removed_object))
                FROM drop_object
            )
            SELECT keys.name, NULL::timestamp with time zone AS created,
                CASE WHEN (queues.removed_object) IS NULL THEN NULL ELSE false END AS delete_marker,
                NULL::timestamp with time zone AS delete_marker_created,
                CASE WHEN (queues.removed_object) IS NULL THEN 'S3K01' ELSE NULL::text END AS error
            FROM keys
            /* JOIN lock/unlock is just for executing CTE */
            LEFT JOIN lock ON keys.name = lock.name
            LEFT JOIN queues on keys.name = (queues.removed_object).name;
        $quote$, i_bid, i_multiple_drop_objects);
    END IF;

END;
$$;
