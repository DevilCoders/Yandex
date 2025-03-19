CREATE OR REPLACE FUNCTION v1_impl.drop_object_parts(
    i_bid uuid,
    i_name text,
    i_object_created timestamp with time zone
) RETURNS void
LANGUAGE plpgsql VOLATILE AS
$function$
BEGIN
    EXECUTE format($quote$
        WITH drop_parts AS (
            DELETE FROM s3.completed_parts as o
            WHERE o.bid=%1$L AND o.name=%2$L AND o.object_created=%3$L
            RETURNING o.*
        )
        SELECT v1_impl.storage_delete_queue_push((select array_agg((
            o.bid, o.name, o.object_created, o.part_id, o.end_offset, o.created,
            o.data_size, o.data_md5, o.mds_couple_id, o.mds_key_version, o.mds_key_uuid,
            o.storage_class, o.encryption
        )::v1_code.completed_part) from drop_parts o));
    $quote$, i_bid, i_name, i_object_created);
END;
$function$;

CREATE OR REPLACE FUNCTION v1_impl.drop_object_parts(
    i_object v1_code.object
) RETURNS void
LANGUAGE plpgsql VOLATILE AS
$function$
BEGIN
    PERFORM v1_impl.drop_object_parts(i_object.bid, i_object.name, i_object.created);
END;
$function$;
