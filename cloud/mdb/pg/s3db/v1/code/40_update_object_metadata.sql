/*
 * Update object (or a current version) meta info.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket to add the object to.
 * - i_name:
 *     Name of the object to update metadata.
 * - i_mds_couple_id, i_mds_key_version, i_mds_key_uuid:
 *     MDS storage information of the stored object's data.
 * - i_storage_class:
 *     Object's storage class.
 * - i_creator_id:
 *     Requester ID.
 * - i_metadata:
 *     Object's S3 metadata.
 * - i_acl:
 *     Object's ACL.
 * - i_created:
 *     The object creation date. If object has different creation date then update will be skipped.
 * - i_update_modified:
 *     true for update object's create date.
 *
 * Returns:
 *   An instance of ``code.object`` that represents updated object.
 */
CREATE OR REPLACE FUNCTION v1_code.update_object_metadata(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_created timestamp with time zone DEFAULT NULL,
    i_update_modified BOOLEAN DEFAULT TRUE
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_object v1_code.object;

    v_s3_first_part s3.object_part;
    v_first_part v1_code.object_part;
    v_data_parts s3.object_part[];

    v_metadata JSONB;
    v_acl JSONB;

    v_meta_update bool;
    v_mds_update bool;

    v_new_modified timestamp with time zone;
    v_chunk_counters_change v1_code.chunk_counters;
BEGIN
    /*
     * Find actual object chunk, block this to share and return this chunk id
     * as object's cid.
     * NOTE: we actually store NULL in object's cid, due to possibly moving
     * objects between chunks in future.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_name);
    PERFORM v1_impl.lock_object(i_bid, i_name);

    /*
     * Block current version of object if exists.
     */
    EXECUTE format($quote$
        SELECT bid, name, created, %4$L /* v_cid */, data_size, data_md5, mds_namespace, mds_couple_id,
            mds_key_version, mds_key_uuid, null_version, delete_marker,
            parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
        FROM s3.objects
        WHERE bid = %1$L /* i_bid */
          AND name = %2$L /* i_name */
          AND created = coalesce(%3$L::timestamptz, created) /* i_created */
        FOR UPDATE
    $quote$, i_bid, i_name, i_created, v_cid) INTO v_object;

    IF v_object IS NULL THEN
        RAISE EXCEPTION 'No such object'
            USING ERRCODE = 'S3K01';
    END IF;

    v_meta_update := (
        i_storage_class IS NOT NULL
        OR i_creator_id IS NOT NULL
        OR i_metadata IS NOT NULL
        OR i_acl IS NOT NULL
    );
    v_mds_update := (
        v_object.mds_couple_id != coalesce(i_mds_couple_id, v_object.mds_couple_id)
        OR v_object.mds_key_version != coalesce(i_mds_key_version, v_object.mds_key_version)
        OR v_object.mds_key_uuid != coalesce(i_mds_key_uuid, v_object.mds_key_uuid)
    ) OR (
        NOT v_meta_update
        AND i_mds_couple_id IS NULL
        AND i_mds_key_version IS NULL
        AND i_mds_key_uuid IS NULL
    ) OR (
        NOT v_meta_update
        AND v_object.mds_couple_id IS NULL
        AND v_object.mds_key_version IS NULL
        AND v_object.mds_key_uuid IS NULL
    );

    IF (coalesce(v_object.parts_count, 0) = 0) AND v_mds_update THEN
        RAISE EXCEPTION 'Simple object storage info update is not supported'
            -- NOTE: 0A000 code is "feature_not_supported"
            USING ERRCODE = '0A000';
    END IF;

    IF v_mds_update
        AND v_object.mds_couple_id IS NOT NULL
        AND v_object.mds_key_version IS NOT NULL
        AND v_object.mds_key_uuid IS NOT NULL
    THEN
        PERFORM v1_impl.storage_delete_queue_push(
            (
             i_bid, v_cid, i_name, v_object.created, 0 /* part_id */,
             current_timestamp /* created */, 0 /* data_size */, v_object.data_md5,
             v_object.mds_namespace, v_object.mds_couple_id, v_object.mds_key_version, v_object.mds_key_uuid,
             v_object.storage_class, null /* metadata */
            )::v1_code.object_part
        );
    END IF;

    v_metadata := v1_impl.merge_object_metadata(i_metadata, v_object.metadata);
    IF v_metadata = '{}'::JSONB THEN
        v_metadata = NULL;
    END IF;

    v_acl := coalesce(i_acl, v_object.acl);
    IF v_acl = '{}'::JSONB THEN
        v_acl = NULL;
    END IF;

    v_new_modified := v_object.created;
    IF i_update_modified IS TRUE THEN
        v_new_modified = current_timestamp;
    END IF;

    IF (i_storage_class IS NOT NULL) AND (v_object.storage_class != i_storage_class) THEN
        PERFORM v1_code.chunks_counters_queue_push(OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(v_object));
        v_chunk_counters_change := v1_code.object_get_chunk_counters(v_object);
        v_chunk_counters_change.storage_class := i_storage_class;
        PERFORM v1_code.chunks_counters_queue_push(v_chunk_counters_change);
    END IF;

    IF v_object.parts_count IS NOT NULL AND v_object.parts IS NULL AND v_new_modified != v_object.created THEN
        EXECUTE format($quote$
            UPDATE s3.completed_parts
            SET object_created = %4$L /* v_new_modified */
            WHERE bid = %1$L /* i_bid */
              AND name = %2$L /* i_name */
              AND object_created = %3$L /* i_created */
        $quote$, i_bid, i_name, v_object.created, v_new_modified);
    END IF;

    EXECUTE format($quote$
        UPDATE s3.objects
        SET
            mds_namespace = %4$L /* i_mds_namespace */,
            mds_couple_id = %5$L /* i_mds_couple_id */,
            mds_key_version = %6$L /* i_mds_key_version */,
            mds_key_uuid = %7$L /* i_mds_key_uuid */,
            storage_class = %8$L /* coalesce(i_storage_class, v_object.storage_class) */,
            creator_id = %9$L /* coalesce(i_creator_id, v_object.creator_id) */,
            metadata = %10$L /* v_metadata */,
            acl = %11$L /* acl */,
            created = %12$L /* v_new_modified */
        WHERE bid = %1$L /* i_bid */
          AND name = %2$L /* i_name */
          AND created = %3$L /* i_created */
        RETURNING bid, name, created, cid, data_size, data_md5, mds_namespace,
            mds_couple_id, mds_key_version, mds_key_uuid, null_version, delete_marker,
            parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
    $quote$, i_bid, i_name, v_object.created,
       case when v_mds_update then i_mds_namespace else v_object.mds_namespace end,
       case when v_mds_update then i_mds_couple_id else v_object.mds_couple_id end,
       case when v_mds_update then i_mds_key_version else v_object.mds_key_version end,
       case when v_mds_update then i_mds_key_uuid else v_object.mds_key_uuid end,
       coalesce(i_storage_class, v_object.storage_class),
       coalesce(i_creator_id, v_object.creator_id),
       v_metadata, v_acl, v_new_modified
    )
        INTO v_object;

    RETURN v_object;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.update_object_metadata(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_mds_couple_id integer DEFAULT NULL,
    i_mds_key_version integer DEFAULT NULL,
    i_mds_key_uuid uuid DEFAULT NULL,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_created timestamp with time zone DEFAULT NULL,
    i_update_modified BOOLEAN DEFAULT TRUE
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_code.update_object_metadata(
        i_bucket_name, i_bid, i_name, /*i_mds_namespace*/ NULL,
        i_mds_couple_id, i_mds_key_version, i_mds_key_uuid, i_storage_class, i_creator_id,
        i_metadata, i_acl, i_created, i_update_modified);
END;
$$;
