/*
 * Returns some or all (up to ``i_limit``) deleted objects available for removal.
 *
 * Args:
 * - i_limit:
 *    Sets the maximum number of keys returned in the response.
 *
 * TODO:
 * - When we can cleanup deleted objects with mds_key_version = 1,
 *   then remove restriction on mds_key_version
 *
 * Returns:
 *   List of the v1_code.deleted_object that satisfy the search criteria
 *   specified by ``i_limit``.
 */
CREATE OR REPLACE FUNCTION v1_code.list_deleted_objects(
    i_limit integer DEFAULT 10,
    i_delay interval DEFAULT '1 week'
) RETURNS SETOF v1_code.deleted_object
LANGUAGE sql
SECURITY DEFINER AS $$
    SELECT bid, name, part_id, data_size, mds_namespace, mds_couple_id,
            mds_key_version, mds_key_uuid, remove_after_ts, storage_class
        FROM s3.storage_delete_queue
        WHERE deleted_ts <= current_timestamp - i_delay
            AND remove_after_ts <= current_timestamp
            AND mds_key_version != 1
        FOR UPDATE SKIP LOCKED
        LIMIT i_limit;
$$;
