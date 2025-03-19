/*
 * Remove a specified deleted object (previously was either object in s3.objects
 * or object part in s3.object_parts) from the storage delete queue.
 *
 * This script must always work together with v1_code.list_delete_objects.
 * FOR UPDATE was already used in v1_code.list_delete_objects, that's why
 * we don't have to use FOR UPDATE when we are finding the record to delete.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket to remove the deleted object from.
 * - i_name:
 *     Name of the deleted object to remove.
 * - i_part_id:
 *     NULL if the deleted object previously was a simple object or
 *     part ID if the deleted object previously was an object part.
 * - i_mds_namespace:
 *     mds_namespace of the deleted object to remove.
 * - i_mds_couple_id:
 *     ID of MDS couple to remove the deleted object from.
 * - i_mds_key_version:
 *     mds_key_version of the deleted object to remove.
 * - i_mds_key_uuid:
 *     mds_key_uuid of the deleted object to remove.
 *
 * TODO:
 * - When we will support deletion records with mds_key_version=1,
 *   then remove the restriction on mds_key_version
 *
 * Returns:
 * - Nothing
 */
CREATE OR REPLACE FUNCTION v1_code.remove_from_storage_delete_queue(
    i_bid uuid,
    i_name text,
    i_part_id integer,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid
) RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER AS $$
DECLARE
    v_deleted_object v1_code.deleted_object;
BEGIN
    PERFORM 1
        FROM s3.storage_delete_queue
        WHERE bid = i_bid
          AND name = i_name
          AND coalesce(part_id, 0) = coalesce(i_part_id, 0)
          AND mds_namespace IS NOT DISTINCT FROM i_mds_namespace
          AND mds_couple_id = i_mds_couple_id
          AND mds_key_version = i_mds_key_version
          AND mds_key_uuid = i_mds_key_uuid
          AND mds_key_version != 1
        LIMIT 1;

    IF NOT FOUND THEN
        RETURN;
    END IF;

    DELETE
        FROM s3.storage_delete_queue
        WHERE bid = i_bid
          AND name = i_name
          AND coalesce(part_id, 0) = coalesce(i_part_id, 0)
          AND mds_namespace IS NOT DISTINCT FROM i_mds_namespace
          AND mds_couple_id = i_mds_couple_id
          AND mds_key_version = i_mds_key_version
          AND mds_key_uuid = i_mds_key_uuid
    RETURNING bid, name, part_id, data_size, mds_namespace, mds_couple_id,
          mds_key_version, mds_key_uuid, remove_after_ts, storage_class
    INTO v_deleted_object;

    PERFORM v1_code.chunks_counters_queue_push(
        OPERATOR(v1_code.-) v1_code.object_deleted_get_chunk_counters(v_deleted_object));
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.remove_from_storage_delete_queue(
    i_bid uuid,
    i_name text,
    i_part_id integer,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid
) RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER AS $$
BEGIN
    PERFORM v1_code.remove_from_storage_delete_queue(
        i_bid, i_name, i_part_id, /*i_mds_namespace*/ NULL,
        i_mds_couple_id, i_mds_key_version, i_mds_key_uuid);
END;
$$;
