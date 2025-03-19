CREATE OR REPLACE FUNCTION v1_code.expire_multiple_noncurrent(
    i_bid uuid,
    i_objects v1_code.lifecycle_element_key[]
) RETURNS SETOF v1_code.lifecycle_result
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format($quote$
        WITH keys AS (
            SELECT name, created
            FROM unnest(%2$L::v1_code.lifecycle_element_key[] /* i_objects */)
        ), drop_noncurrent as (
            DELETE FROM s3.objects_noncurrent as o
                USING keys as k
                WHERE o.bid = %1$L /* i_bid */
                    AND o.name = k.name
                    AND o.created = k.created
            RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), o.data_size,
                o.data_md5, NULL /*mds_namespace*/, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                o.null_version, o.delete_marker, o.parts_count, o.parts, o.storage_class, o.creator_id,
                o.metadata, o.acl, o.lock_settings)::v1_code.object as removed_object, 2 as remove_order
        ), queues as (
            SELECT removed_object,
                v1_impl.storage_delete_queue_push(removed_object),
                v1_impl.drop_object_parts(removed_object),
                v1_code.chunks_counters_queue_push(
                        OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(removed_object))
            FROM drop_noncurrent
        )
        SELECT (removed_object).name, (removed_object).created, NULL::text as error
        FROM queues;
    $quote$, i_bid, i_objects);
END;
$$;
