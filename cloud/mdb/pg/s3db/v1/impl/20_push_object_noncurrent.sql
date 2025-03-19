CREATE OR REPLACE FUNCTION v1_impl.push_object_noncurrent(
    i_obj v1_code.object
)
RETURNS void
LANGUAGE plpgsql VOLATILE AS
$function$
BEGIN
    EXECUTE format($quote$
        INSERT INTO s3.objects_noncurrent (
            bid, name, created, data_size, data_md5, mds_couple_id,
            mds_key_version, mds_key_uuid, null_version, delete_marker,
            parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
        )
        VALUES (
            %1$L, %2$L, %3$L, %4$L, %5$L, %6$L, %7$L, %8$L, %9$L, %10$L, %11$L, %12$L,
            %13$L, %14$L, %15$L, %16$L, %17$L
        )
    $quote$, i_obj.bid, i_obj.name, i_obj.created, i_obj.data_size, i_obj.data_md5, i_obj.mds_couple_id,
        i_obj.mds_key_version, i_obj.mds_key_uuid, i_obj.null_version, i_obj.delete_marker, i_obj.parts_count,
        i_obj.parts, i_obj.storage_class, i_obj.creator_id, i_obj.metadata, i_obj.acl, i_obj.lock_settings);
END;
$function$;
