CREATE OR REPLACE FUNCTION v1_impl.remove_null_object_noncurrent(
    i_bid uuid,
    i_cid bigint,
    i_name text
)
RETURNS void
LANGUAGE plpgsql VOLATILE AS
$function$
BEGIN
    EXECUTE format($quote$
        WITH drop_noncurrent as (
            DELETE FROM s3.objects_noncurrent
                WHERE bid = %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND null_version
            RETURNING (bid, name, created, %3$L /* i_cid */, data_size,
                data_md5, NULL /*mds_namespace*/, mds_couple_id, mds_key_version, mds_key_uuid,
                null_version, delete_marker, parts_count, parts, storage_class, creator_id,
                metadata, acl, lock_settings)::v1_code.object as removed_object
        )
        SELECT removed_object,
            v1_impl.storage_delete_queue_push(removed_object),
            v1_impl.drop_object_parts(removed_object),
            v1_code.chunks_counters_queue_push(
                    OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(removed_object))
        FROM drop_noncurrent
    $quote$, i_bid, i_name, i_cid);
END
$function$;
