CREATE OR REPLACE FUNCTION v1_code.cleanup_delete_markers(
    i_bid uuid,
    i_objects v1_code.lifecycle_element_key[]
) RETURNS SETOF v1_code.lifecycle_result
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format($quote$
        WITH keys AS (
            SELECT name, created
            FROM unnest(%2$L::v1_code.lifecycle_element_key[] /* i_objects */) as k
            WHERE NOT EXISTS(
                SELECT 1 FROM s3.objects_noncurrent o
                WHERE o.bid = %1$L /* i_bid */
                    AND o.name = k.name
            )
        ), drop_del_mark as (
            DELETE FROM s3.object_delete_markers as o
               USING keys as k
               WHERE o.bid = %1$L /* i_bid */
                    AND o.name = k.name
                    AND o.created = k.created
            RETURNING (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), NULL /*data_size*/,
                NULL /*data_md5*/, NULL /*mds_namespace*/, NULL /*mds_couple_id*/,
                NULL /*mds_key_version*/, NULL /*mds_key_uuid*/, o.null_version,
                true /*delete_marker*/, NULL /*parts_count*/, NULL /*parts*/, NULL /*storage_class*/,
                creator_id, NULL /*metadata*/, NULL /*acl*/, NULL /*lock_settings*/)::v1_code.object as removed_object
        ), queues as (
            SELECT removed_object,
                v1_code.chunks_counters_queue_push(
                        OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(removed_object))
            FROM drop_del_mark
        )
        SELECT (removed_object).name, (removed_object).created, NULL::text as error
        FROM queues;
    $quote$, i_bid, i_objects);
END;
$$;
