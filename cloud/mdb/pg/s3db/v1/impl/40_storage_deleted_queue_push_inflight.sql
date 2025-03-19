/*
 * Add inflight record to delete_storage_queue.
 * 
 * TODO: comments
 */
CREATE OR REPLACE FUNCTION v1_impl.storage_delete_queue_push_inflight(
    i_inflight v1_code.inflight_result
) RETURNS void
LANGUAGE plpgsql AS $$
BEGIN
    IF i_inflight.mds_couple_id IS NOT NULL
        AND i_inflight.mds_key_version IS NOT NULL
        AND i_inflight.mds_key_uuid IS NOT NULL
    THEN
        INSERT INTO s3.storage_delete_queue (
            bid, name, part_id, data_size,
            mds_namespace, mds_couple_id, mds_key_version,
            mds_key_uuid, created, deleted_ts,
            remove_after_ts, data_md5, parts_count,
            storage_class
        ) VALUES (
            i_inflight.bid, i_inflight.object_name, i_inflight.part_id,
            0 /* datasize */, NULL/* mds_namespace */,
            i_inflight.mds_couple_id, i_inflight.mds_key_version, i_inflight.mds_key_uuid,
            i_inflight.object_created, current_timestamp,
            current_timestamp, NULL, NULL, NULL /* storage_class */
        );

        PERFORM v1_code.chunks_counters_queue_push(v1_code.object_deleted_get_chunk_counters((
          i_inflight.bid, i_inflight.object_name, i_inflight.part_id,
          0 /* data_size */, NULL /* mds_namespace */, 
          i_inflight.mds_couple_id, i_inflight.mds_key_version, i_inflight.mds_key_uuid,
          current_timestamp, NULL)::v1_code.deleted_object));
    END IF;
END;
$$;
