CREATE OR REPLACE FUNCTION v1_code.transfer_multiple_noncurrent(
    i_bid uuid,
    i_storage_class int,
    i_objects v1_code.lifecycle_element_key[]
) RETURNS SETOF v1_code.lifecycle_result
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format($quote$
        WITH keys AS (
            SELECT name, created
            FROM unnest(%2$L::v1_code.lifecycle_element_key[] /* i_objects */)
        ), objects AS (
            SELECT (o.bid, o.name, o.created, v1_code.get_object_cid(%1$L /* i_bid */, o.name), o.data_size,
                o.data_md5, NULL /* mds_namespace */, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
                o.null_version, o.delete_marker, o.parts_count, o.parts, o.storage_class, o.creator_id,
                o.metadata, o.acl, o.lock_settings)::v1_code.object as obj
            FROM s3.objects_noncurrent o
                INNER JOIN keys as k ON o.bid = %1$L /* i_bid */
                    AND o.name = k.name
                    AND o.created = k.created
            WHERE coalesce(storage_class, 0) != coalesce(%3$L /* i_storage_class */, 0)
        ), updates as (
            UPDATE s3.objects_noncurrent u
                SET storage_class = coalesce(%3$L /* i_storage_class */, 0)
            FROM objects AS o
            WHERE u.bid = %1$L /* i_bid */ AND (o.obj).name = u.name AND (o.obj).created = u.created
            RETURNING (u.bid, u.name, u.created, v1_code.get_object_cid(%1$L /* i_bid */, u.name), u.data_size,
                u.data_md5, NULL /* mds_namespace */, u.mds_couple_id, u.mds_key_version, u.mds_key_uuid,
                u.null_version, u.delete_marker, u.parts_count, u.parts, u.storage_class, u.creator_id,
                u.metadata, u.acl, u.lock_settings)::v1_code.object as obj
        ), queues as (
            SELECT obj, false as updated,
                v1_code.chunks_counters_queue_push(
                        OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(obj))
            FROM objects
            UNION ALL
            SELECT obj, true as updated,
                v1_code.chunks_counters_queue_push(v1_code.object_get_chunk_counters(obj))
            FROM updates
        )
        SELECT (obj).name, (obj).created, NULL::text as error
        FROM queues
        WHERE updated;
    $quote$, i_bid, i_objects, i_storage_class);
END;
$$;
