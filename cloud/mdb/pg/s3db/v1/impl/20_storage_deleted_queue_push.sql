CREATE OR REPLACE FUNCTION v1_impl.storage_delete_queue_push(
    i_parts v1_code.completed_part[]
)
RETURNS void
LANGUAGE plpgsql VOLATILE AS
$function$
BEGIN
    INSERT INTO s3.storage_delete_queue (
        bid, name, part_id, data_size,
        mds_namespace, mds_couple_id, mds_key_version,
        mds_key_uuid, created, deleted_ts,
        remove_after_ts, data_md5, parts_count, storage_class, metadata
    )
    SELECT o.bid, o.name,
        o.part_id, o.data_size, NULL /*mds_namespace*/, o.mds_couple_id,
        o.mds_key_version, o.mds_key_uuid, o.object_created, current_timestamp,
        current_timestamp, o.data_md5,
        COUNT(*) OVER (PARTITION BY o.bid, o.name, o.object_created) /* parts_count */,
        o.storage_class, CASE WHEN o.encryption IS NOT NULL
            THEN jsonb_build_object('encryption', jsonb_build_object('meta', o.encryption))
            ELSE NULL END
    FROM (SELECT (unnest(i_parts)).*) o
    WHERE o.mds_couple_id IS NOT NULL
        AND o.mds_key_version IS NOT NULL
        AND o.mds_key_uuid IS NOT NULL;

    PERFORM v1_code.chunks_counters_queue_push(v1_code.object_deleted_get_chunk_counters((
        o.bid, o.name, o.part_id, o.data_size, NULL /*mds_namespace*/, o.mds_couple_id,
        o.mds_key_version, o.mds_key_uuid, current_timestamp, o.storage_class)::v1_code.deleted_object))
    FROM (SELECT (unnest(i_parts)).*) o;
END
$function$;

CREATE OR REPLACE FUNCTION v1_impl.storage_delete_queue_push(
    i_part v1_code.object_part
)
RETURNS void
LANGUAGE plpgsql VOLATILE AS
$function$
BEGIN
    IF i_part.mds_couple_id IS NOT NULL
        AND i_part.mds_key_version IS NOT NULL
        AND i_part.mds_key_uuid IS NOT NULL
    THEN
        INSERT INTO s3.storage_delete_queue (
            bid, name, part_id, data_size,
            mds_namespace, mds_couple_id, mds_key_version,
            mds_key_uuid, created, deleted_ts,
            remove_after_ts, data_md5, parts_count, storage_class, metadata
        )
        VALUES (i_part.bid, i_part.name, i_part.part_id,
                i_part.data_size, i_part.mds_namespace, i_part.mds_couple_id,
                i_part.mds_key_version, i_part.mds_key_uuid,
                i_part.object_created, current_timestamp,
                current_timestamp, i_part.data_md5, NULL, i_part.storage_class, i_part.metadata
        );

        PERFORM v1_code.chunks_counters_queue_push(v1_code.object_deleted_get_chunk_counters((
          i_part.bid, i_part.name, i_part.part_id, i_part.data_size, i_part.mds_namespace, i_part.mds_couple_id,
          i_part.mds_key_version, i_part.mds_key_uuid, current_timestamp, i_part.storage_class)::v1_code.deleted_object));
    END IF;
END
$function$;

CREATE OR REPLACE FUNCTION v1_impl.storage_delete_queue_push(
    i_obj v1_code.object
)
RETURNS void
LANGUAGE plpgsql VOLATILE AS
$function$
BEGIN
    IF i_obj.parts_count IS NULL THEN
        -- simple object
        IF i_obj.mds_couple_id IS NOT NULL
            AND i_obj.mds_key_version IS NOT NULL
            AND i_obj.mds_key_uuid IS NOT NULL
        THEN
            INSERT INTO s3.storage_delete_queue (
                    bid, name, part_id, data_size,
                    mds_namespace, mds_couple_id, mds_key_version,
                    mds_key_uuid, created, deleted_ts,
                    remove_after_ts, data_md5, parts_count, storage_class,
                    creator_id, metadata, acl
                )
                VALUES (
                    i_obj.bid, i_obj.name, NULL,
                    i_obj.data_size, i_obj.mds_namespace, i_obj.mds_couple_id,
                    i_obj.mds_key_version, i_obj.mds_key_uuid,
                    i_obj.created, current_timestamp,
                    current_timestamp, i_obj.data_md5,
                    NULL, i_obj.storage_class,
                    i_obj.creator_id, i_obj.metadata, i_obj.acl
                );

            PERFORM v1_code.chunks_counters_queue_push(v1_code.object_deleted_get_chunk_counters((
              i_obj.bid, i_obj.name, NULL, i_obj.data_size, i_obj.mds_namespace, i_obj.mds_couple_id,
              i_obj.mds_key_version, i_obj.mds_key_uuid, current_timestamp, i_obj.storage_class)::v1_code.deleted_object));
        END IF;
    ELSE
        -- multipart object
        IF i_obj.mds_couple_id IS NOT NULL
            AND i_obj.mds_key_version IS NOT NULL
            AND i_obj.mds_key_uuid IS NOT NULL
        THEN
            -- multipart "root" record with metadata
            INSERT INTO s3.storage_delete_queue (
                bid, name, part_id, data_size,
                mds_namespace, mds_couple_id, mds_key_version,
                mds_key_uuid, created, deleted_ts,
                remove_after_ts, data_md5, parts_count, storage_class,
                creator_id, metadata, acl
            )
            VALUES (
                i_obj.bid, i_obj.name, 0, 0,
                i_obj.mds_namespace, i_obj.mds_couple_id, i_obj.mds_key_version,
                i_obj.mds_key_uuid, i_obj.created,
                current_timestamp, current_timestamp,
                i_obj.data_md5, i_obj.parts_count, i_obj.storage_class,
                i_obj.creator_id, i_obj.metadata, i_obj.acl
            );

            PERFORM v1_code.chunks_counters_queue_push(v1_code.object_deleted_get_chunk_counters((
              i_obj.bid, i_obj.name, 0, 0, i_obj.mds_namespace, i_obj.mds_couple_id,
              i_obj.mds_key_version, i_obj.mds_key_uuid, current_timestamp, i_obj.storage_class)::v1_code.deleted_object));
        END IF;

        INSERT INTO s3.storage_delete_queue (
            bid, name, part_id, data_size,
            mds_namespace, mds_couple_id, mds_key_version,
            mds_key_uuid, created, deleted_ts,
            remove_after_ts, data_md5, parts_count, storage_class, metadata
        )
        SELECT i_obj.bid, i_obj.name, a.part_id,
            data_size, /*mds_namespace*/ NULL, mds_couple_id, mds_key_version,
            mds_key_uuid, i_obj.created,
            current_timestamp, current_timestamp,
            data_md5, i_obj.parts_count, i_obj.storage_class,
            CASE WHEN e.encryption IS NOT NULL
                THEN jsonb_build_object('encryption', jsonb_build_object('meta', e.encryption))
                ELSE NULL END
        FROM (
            SELECT (unnest(i_obj.parts)).*
        ) a
        LEFT JOIN (
            SELECT *
            FROM jsonb_array_elements_text(i_obj.metadata->'encryption'->'parts_meta')
            WITH ORDINALITY AS t (encryption, part_id)
        ) e
        ON e.part_id=a.part_id
        WHERE mds_couple_id IS NOT NULL AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL;

        PERFORM v1_code.chunks_counters_queue_push(v1_code.object_deleted_get_chunk_counters((
            i_obj.bid, i_obj.name, part_id,
            data_size, /*mds_namespace*/ NULL, mds_couple_id, mds_key_version,
            mds_key_uuid, current_timestamp, i_obj.storage_class)::v1_code.deleted_object))
        FROM (
            SELECT (unnest(i_obj.parts)).*
        ) a;
    END IF;
END
$function$;
