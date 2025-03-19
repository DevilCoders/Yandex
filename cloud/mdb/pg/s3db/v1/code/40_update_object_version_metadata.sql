/*
 * Update object's version meta info.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket to add the object to.
 * - i_name:
 *     Name of the object to update metadata.
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
 *
 * NOTE: If version is delete marker then no update will be performed.
 *
 * Returns:
 *   An instance of ``code.object`` that represents updated object.
 */
 CREATE OR REPLACE FUNCTION v1_code.update_object_version_metadata(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_created timestamp with time zone DEFAULT NULL
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_object v1_code.object;

    v_metadata JSONB;
    v_acl JSONB;

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
     * Block object's version if exists.
     */
    EXECUTE format($quote$
        SELECT bid, name, created, %4$L /* v_cid */, data_size, data_md5, mds_namespace, mds_couple_id,
            mds_key_version, mds_key_uuid, null_version, delete_marker,
            parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
        FROM s3.objects
        WHERE bid = %1$L /* i_bid */
          AND name = %2$L /* i_name */
          AND (%3$L IS NULL and null_version
            OR NOT null_version AND created = %3$L::timestamptz /* i_created */)
        UNION ALL
        SELECT bid, name, created, %4$L /* v_cid */, data_size, data_md5, NULL as mds_namespace, mds_couple_id,
            mds_key_version, mds_key_uuid, null_version, delete_marker,
            parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
        FROM s3.objects_noncurrent
        WHERE bid = %1$L /* i_bid */
          AND name = %2$L /* i_name */
          AND NOT delete_marker
          AND (%3$L IS NULL and null_version
            OR NOT null_version AND created = %3$L::timestamptz /* i_created */)
    $quote$, i_bid, i_name, i_created, v_cid) INTO v_object;

    IF v_object IS NULL THEN
        RAISE EXCEPTION 'No such object'
            USING ERRCODE = 'S3K01';
    END IF;

    v_metadata := v1_impl.merge_object_metadata(i_metadata, v_object.metadata);
    IF v_metadata = '{}'::JSONB THEN
        v_metadata = NULL;
    END IF;

    v_acl := coalesce(i_acl, v_object.acl);
    IF v_acl = '{}'::JSONB THEN
        v_acl = NULL;
    END IF;

    IF (i_storage_class IS NOT NULL) AND (v_object.storage_class != i_storage_class) THEN
        PERFORM v1_code.chunks_counters_queue_push(OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(v_object));
        v_chunk_counters_change := v1_code.object_get_chunk_counters(v_object);
        v_chunk_counters_change.storage_class := i_storage_class;
        PERFORM v1_code.chunks_counters_queue_push(v_chunk_counters_change);
    END IF;

    EXECUTE format($quote$
        WITH current as (
            UPDATE s3.objects
            SET
                storage_class = %4$L /* coalesce(i_storage_class, v_object.storage_class) */,
                creator_id = %5$L /* coalesce(i_creator_id, v_object.creator_id) */,
                metadata = %6$L /* v_metadata */,
                acl = %7$L /* acl */
            WHERE bid = %1$L /* i_bid */
            AND name = %2$L /* i_name */
            AND created = %3$L /* i_created */
            RETURNING (bid, name, created, cid, data_size, data_md5, mds_namespace,
                mds_couple_id, mds_key_version, mds_key_uuid, null_version, delete_marker,
                parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings)::v1_code.object as updated
        ), noncurrent as (
            UPDATE s3.objects_noncurrent
            SET
                storage_class = %4$L /* coalesce(i_storage_class, v_object.storage_class) */,
                creator_id = %5$L /* coalesce(i_creator_id, v_object.creator_id) */,
                metadata = %6$L /* v_metadata */,
                acl = %7$L /* acl */
            WHERE bid = %1$L /* i_bid */
            AND name = %2$L /* i_name */
            AND created = %3$L /* i_created */
            RETURNING (bid, name, created, NULL /* cid*/, data_size, data_md5, NULL /*mds_namespace*/,
                mds_couple_id, mds_key_version, mds_key_uuid, null_version, delete_marker,
                parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings)::v1_code.object  as updated
        )
        SELECT (updated).*
            FROM (
                SELECT updated FROM current
                UNION ALL
                SELECT updated FROM noncurrent
            ) AS a
    $quote$, i_bid, i_name, v_object.created,
       coalesce(i_storage_class, v_object.storage_class),
       coalesce(i_creator_id, v_object.creator_id),
       v_metadata, v_acl
    ) INTO v_object;

    RETURN v_object;
END;
$$;
