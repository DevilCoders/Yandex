CREATE OR REPLACE FUNCTION v1_impl.drop_object_current(
    i_bid uuid,
    i_cid bigint,
    i_name text,
    i_created timestamp with time zone
)
RETURNS SETOF v1_code.object
LANGUAGE plpgsql VOLATILE AS
$function$
BEGIN
    RETURN QUERY EXECUTE format($quote$
        WITH drop_object as (
            DELETE FROM s3.objects
                WHERE bid =  %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND (%3$L is NULL OR created = %3$L) /* i_created */
            RETURNING (bid, name, created, %4$L /* i_cid */, data_size,
                data_md5, mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
                null_version, false /*delete_marker*/, parts_count, parts, storage_class, creator_id,
                metadata, acl, lock_settings)::v1_code.object as removed_object, 1 as remove_order
        ), drop_del_mark as (
            DELETE FROM s3.object_delete_markers
                WHERE bid =  %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND (%3$L is NULL OR created = %3$L) /* i_created */
            RETURNING (bid, name, created, %4$L /* i_cid */, NULL /*data_size*/,
                NULL /*data_md5*/, NULL /*mds_namespace*/, NULL /*mds_couple_id*/,
                NULL /*mds_key_version*/, NULL /*mds_key_uuid*/, null_version,
                true /*delete_marker*/, NULL /*parts_count*/, NULL /*parts*/, NULL /*storage_class*/,
                creator_id, NULL /*metadata*/, NULL /*acl*/, NULL /*lock_settings*/)::v1_code.object as removed_object, 1 as remove_order
        )
        SELECT (removed_object).*
        FROM (
            SELECT removed_object, remove_order FROM drop_object
            UNION ALL
            SELECT removed_object, remove_order FROM drop_del_mark
        ) as a
    $quote$, i_bid, i_name, i_created, i_cid);
END;
$function$;
