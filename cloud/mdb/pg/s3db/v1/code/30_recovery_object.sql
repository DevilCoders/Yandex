CREATE OR REPLACE FUNCTION v1_code.recovery_object(
    i_bid uuid,
    i_name text,
    i_deleted_from timestamp with time zone DEFAULT NULL
) RETURNS SETOF v1_code.object
LANGUAGE plpgsql AS $$
DECLARE
    v_removed_object v1_code.object;
    v_first_part v1_code.multipart_upload;
    v_parts_correct_order boolean;
    v_chunk_counters_change v1_code.chunk_counters;
BEGIN
    SELECT bid, NULL as cid, name, created as object_created, part_id, created, data_size, data_md5, mds_namespace,
        mds_couple_id, mds_key_version, mds_key_uuid, storage_class, creator_id, metadata, acl
      FROM s3.storage_delete_queue
      WHERE bid = i_bid AND name = i_name
        AND (i_deleted_from IS NULL OR i_deleted_from < deleted_ts)
      ORDER BY part_id ASC, deleted_ts DESC
      LIMIT 1
      INTO v_first_part
      FOR UPDATE;

    IF v_first_part.part_id is NULL THEN /* simple */
        SELECT bid, name, created, NULL as cid, data_size, data_md5, mds_namespace, mds_couple_id,
            mds_key_version, mds_key_uuid, true as null_version, false as delete_marker, parts_count, NULL as parts,
            storage_class, creator_id, metadata, acl
          INTO v_removed_object
          FROM s3.storage_delete_queue
          WHERE bid = i_bid AND name = i_name
            AND (i_deleted_from IS NULL OR i_deleted_from < deleted_ts)
            AND part_id is NULL
          ORDER BY created DESC
          LIMIT 1
          FOR UPDATE;
    ELSE  /* multipart */
        IF v_first_part.part_id = 0 THEN  /* multipart with root part */
            SELECT i_bid, i_name, max(created) as created, NULL as cid, sum(data_size) as data_size, null as data_md5,
                null as mds_namespace, null as mds_couple_id, null as mds_key_version, null as mds_key_uuid,
                true as null_version, false as delete_marker, count(*) as parts_count,
                array_agg((part_id, created, data_size, data_md5, mds_couple_id, mds_key_version, mds_key_uuid)::s3.object_part) as parts,
                v_first_part.storage_class, v_first_part.creator_id,
                (v_first_part.metadata || (case when bool_or((s.metadata->'encryption'->'meta') is not null)
                    then jsonb_build_object('encryption',
                        coalesce(v_first_part.metadata->'encryption', '{}'::jsonb) ||
                        jsonb_build_object('parts_meta', jsonb_agg(s.metadata->'encryption'->'meta')))
                    else '{}'::jsonb end)) as metadata,
                v_first_part.acl
              FROM (
                  SELECT *
                    FROM s3.storage_delete_queue
                    WHERE bid = i_bid AND name = i_name AND part_id != 0
                      AND created = v_first_part.created
                      AND (i_deleted_from IS NULL OR i_deleted_from < deleted_ts)
                    ORDER BY part_id
                    FOR UPDATE
              ) s
              INTO v_removed_object;
            v_removed_object.data_md5 = v_first_part.data_md5;
            v_removed_object.mds_namespace = v_first_part.mds_namespace;
            v_removed_object.mds_couple_id = v_first_part.mds_couple_id;
            v_removed_object.mds_key_version = v_first_part.mds_key_version;
            v_removed_object.mds_key_uuid = v_first_part.mds_key_uuid;
        ELSE  /* multipart without root part */
            SELECT i_bid, i_name, max(created) as created, NULL as cid, sum(data_size) as data_size,
                md5(string_agg(uuid_send(data_md5), ''::bytea order by part_id))::uuid as data_md5,
                null as mds_namespace, null as mds_couple_id, null as mds_key_version, null as mds_key_uuid,
                true as null_version, false as delete_marker, count(*) as parts_count,
                array_agg((part_id, created, data_size, data_md5, mds_couple_id, mds_key_version, mds_key_uuid)::s3.object_part) as parts,
                NULL as storage_class, NULL as creator_id,
                (case when bool_or((s.metadata->'encryption'->'meta') is not null)
                    then jsonb_build_object('encryption', jsonb_build_object('parts_meta', jsonb_agg(s.metadata->'encryption'->'meta')))
                    else null end) as metadata,
                NULL as metadata,
                NULL as acl
              FROM (
                  SELECT *
                    FROM s3.storage_delete_queue
                    WHERE bid = i_bid AND name = i_name
                      AND created = v_first_part.created
                      AND (i_deleted_from IS NULL OR i_deleted_from < deleted_ts)
                    ORDER BY part_id
                    FOR UPDATE
              ) s
              INTO v_removed_object;
        END IF;

        SELECT (min_diff = 1 AND max_diff = 1
            AND min_part = 1 AND max_part = cnt)
          INTO v_parts_correct_order
          FROM (
              SELECT min(current - previous) AS min_diff,
                     max(current - previous) AS max_diff,
                     min(current) AS min_part,
                     max(current) AS max_part,
                     count(1) AS cnt
                  FROM (
                      SELECT (p).part_id AS current,
                             lag((p).part_id, 1, 0) OVER () AS previous
                          FROM unnest(v_removed_object.parts) p
                      ) a
              ) b;

        IF NOT v_parts_correct_order THEN
            RAISE EXCEPTION 'Multipart upload parts are in invalid order'
                USING ERRCODE = 'S3M03';
        END IF;
    END IF;

    SELECT i_bid,
        NULL,
        0 /* simple_objects_count */,
        0 /* simple_objects_size */,
        0 /* multipart_objects_count */,
        0 /* multipart_objects_size */,
        0 /* objects_parts_count */,
        0 /* objects_parts_size */,
        0 /* deleted_objects_count */,
        0 /* deleted_objects_size */,
        0 /* active_multipart_count */,
        NULL /* storage_class */
        INTO v_chunk_counters_change;

    WITH deleted_rows AS (
      DELETE FROM s3.storage_delete_queue
        WHERE bid = i_bid AND name = i_name
          AND created = v_first_part.created
          AND (i_deleted_from IS NULL OR i_deleted_from < deleted_ts)
      RETURNING data_size, storage_class
    ) SELECT storage_class, -coalesce(count(*), 0)::bigint, -coalesce(sum(data_size), 0)::bigint
    FROM deleted_rows
    GROUP BY storage_class
    INTO v_chunk_counters_change.storage_class,
      v_chunk_counters_change.deleted_objects_count,
      v_chunk_counters_change.deleted_objects_size;

    v_chunk_counters_change.cid = v1_code.get_object_cid(i_bid, i_name);
    PERFORM v1_code.chunks_counters_queue_push(v_chunk_counters_change);

    IF NOT v_removed_object IS NULL THEN
        PERFORM v1_code.add_object(
            NULL,   /* i_bucket_name */
            v_removed_object.bid,
            'disabled',
            v_removed_object.name,
            NULL,   /* i_cid */
            v_removed_object.data_size,
            v_removed_object.data_md5,
            v_removed_object.mds_namespace,
            v_removed_object.mds_couple_id,
            v_removed_object.mds_key_version,
            v_removed_object.mds_key_uuid,
            v_removed_object.parts,
            v_removed_object.storage_class,
            v_removed_object.creator_id,
            v_removed_object.metadata,
            v_removed_object.acl,
            false   /* i_separate_parts */,
            NULL::JSONB /* i_lock_settings */
        );
    END IF;

    RETURN NEXT v_removed_object;
END;
$$;
