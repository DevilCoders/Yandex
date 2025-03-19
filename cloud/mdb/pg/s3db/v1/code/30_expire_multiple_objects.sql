CREATE OR REPLACE FUNCTION v1_code.expire_multiple_objects(
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_objects v1_code.lifecycle_element_key[]
) RETURNS SETOF v1_code.lifecycle_result
LANGUAGE plpgsql AS $$
BEGIN
    IF i_versioning != 'disabled' THEN
        RETURN QUERY EXECUTE format($quote$
            WITH keys AS (
                SELECT name, created
                FROM unnest(%2$L::v1_code.lifecycle_element_key[] /* i_objects */)
            ), drop_object AS (
                DELETE FROM s3.objects o
                    USING keys as k
                    WHERE o.bid = %1$L /* i_bid */
                        AND o.name = k.name
                        AND o.created = k.created
                RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), o.data_size,
                    o.data_md5, o.mds_namespace, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                    o.null_version, false  /*delete_marker*/, o.parts_count, o.parts, o.storage_class, o.creator_id,
                    o.metadata, o.acl, o.lock_settings)::v1_code.object as removed_object, o.name as name
            ), add_del_mark as (
                INSERT INTO s3.object_delete_markers (bid, name, null_version)
                SELECT
                    (removed_object).bid,
                    (removed_object).name,
                    %3$L/* i_versioning */!='enabled'::s3.bucket_versioning_type
                FROM drop_object d
                RETURNING (
                    bid, name, created, creator_id, null_version
                )::v1_code.object_delete_marker as marker, name
            ), queues as (
                SELECT
                    (removed_object),
                    v1_code.chunks_counters_queue_push(
                        v1_code.delete_marker_get_chunk_counters((removed_object).cid, add_del_mark.marker)
                    ),
                    CASE WHEN
                        %3$L/* i_versioning */='enabled'::s3.bucket_versioning_type
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
                    INNER JOIN drop_object on add_del_mark.name = drop_object.name
            )
            SELECT (removed_object).name, (removed_object).created, NULL::text as error
            FROM queues;
        $quote$, i_bid, i_objects, i_versioning);
    ELSE
        RETURN QUERY EXECUTE format($quote$
            WITH keys AS (
                SELECT name, created
                FROM unnest(%2$L::v1_code.lifecycle_element_key[] /* i_objects */)
            ), drop_object AS (
                DELETE FROM s3.objects as o
                    USING keys as k
                    WHERE o.bid = %1$L /* i_bid */
                        AND o.name = k.name
                        AND o.created = k.created
                RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), o.data_size,
                    o.data_md5, o.mds_namespace, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                    o.null_version, false  /*delete_marker*/, o.parts_count, o.parts, o.storage_class, o.creator_id,
                    o.metadata, o.acl, o.lock_settings)::v1_code.object as removed_object
            ), drop_noncurrent as (
                DELETE FROM s3.objects_noncurrent as o
                    USING keys as k
                    WHERE o.bid = %1$L /* i_bid */
                        AND o.name = k.name
                        AND o.created = k.created
                RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), o.data_size,
                    o.data_md5, NULL /*mds_namespace*/, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                    o.null_version, o.delete_marker, o.parts_count, o.parts, o.storage_class, o.creator_id,
                    o.metadata, o.acl, o.lock_settings)::v1_code.object as removed_object
            ), queues as (
                SELECT removed_object,
                    v1_impl.storage_delete_queue_push(removed_object),
                    v1_impl.drop_object_parts(removed_object),
                    v1_code.chunks_counters_queue_push(
                            OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(removed_object))
                FROM (
                    SELECT removed_object FROM drop_object
                    UNION ALL
                    SELECT removed_object FROM drop_noncurrent
                ) as a
            )
            SELECT (removed_object).name, (removed_object).created, NULL::text as error
            FROM queues;
        $quote$, i_bid, i_objects);
    END IF;

END;
$$;
