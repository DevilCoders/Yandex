/*
 * Remove concrete object's version.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket to add the object to.
 * - i_name:
 *     Name of the object to add.
 * - i_created:
 *     Version creation date or NULL for null version.
 *
 * Returns:
 *   An instance of ``v1_code.object_version`` that represents removed object's version.
 */
 CREATE OR REPLACE FUNCTION v1_code.drop_object_version(
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone DEFAULT NULL
) RETURNS SETOF v1_code.object_version
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_removed_version v1_code.object_version;
BEGIN
    /* Block chunk to share, return this chunk id as object's cid */
    v_cid := v1_code.get_object_cid(i_bid, i_name);
    PERFORM v1_impl.lock_object(i_bid, i_name);

    EXECUTE format($quote$
        WITH drop_object as (
            DELETE FROM s3.objects
                WHERE bid =  %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND (
                        %3$L /* i_created */ IS NULL AND null_version
                        OR
                        %3$L /* i_created */ = created AND NOT null_version
                    )
            RETURNING (bid, name, created, %4$L /* v_cid */, data_size,
                data_md5, mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid,
                null_version, false  /*delete_marker*/, parts_count, parts, storage_class, creator_id,
                metadata, acl, lock_settings)::v1_code.object as removed_object
        ), drop_del_mark as (
            DELETE FROM s3.object_delete_markers
                WHERE bid =  %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND (
                        %3$L /* i_created */ IS NULL AND null_version
                        OR
                        %3$L /* i_created */ = created AND NOT null_version
                    )
            RETURNING (bid, name, created, %4$L /* v_cid */, NULL /*data_size*/,
                NULL /*data_md5*/, NULL /*mds_namespace*/, NULL /*mds_couple_id*/,
                NULL /*mds_key_version*/, NULL /*mds_key_uuid*/, null_version,
                true /*delete_marker*/, NULL /*parts_count*/, NULL /*parts*/, NULL /*storage_class*/,
                creator_id, NULL /*metadata*/, NULL /*acl*/, NULL /*lock_settings*/)::v1_code.object as removed_object
        ), drop_noncurrent as (
            DELETE FROM s3.objects_noncurrent
                WHERE bid =  %1$L /* i_bid */
                    AND name = %2$L /* i_name */
                    AND (
                        %3$L /* i_created */ IS NULL AND null_version
                        OR
                        %3$L /* i_created */ = created AND NOT null_version
                    )
            RETURNING (bid, name, created, %4$L /* v_cid */, data_size,
                data_md5, NULL /*mds_namespace*/, mds_couple_id, mds_key_version, mds_key_uuid,
                null_version, delete_marker, parts_count, parts, storage_class, creator_id,
                metadata, acl, lock_settings)::v1_code.object as removed_object, noncurrent
        ), queues as (
            SELECT
                v1_impl.storage_delete_queue_push(removed_object),
                v1_impl.drop_object_parts(removed_object),
                v1_code.chunks_counters_queue_push(
                    OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(removed_object)),
                (
                    (removed_object).bid,
                    (removed_object).name,
                    (removed_object).created,
                    noncurrent,
                    (removed_object).data_size,
                    (removed_object).data_md5,
                    (removed_object).mds_couple_id,
                    (removed_object).mds_key_version,
                    (removed_object).mds_key_uuid,
                    (removed_object).null_version,
                    (removed_object).delete_marker,
                    (removed_object).parts_count,
                    (removed_object).parts,
                    (removed_object).storage_class,
                    (removed_object).creator_id,
                    (removed_object).metadata,
                    (removed_object).acl,
                    (removed_object).lock_settings
                )::v1_code.object_version as removed_version
            FROM (
                SELECT removed_object, noncurrent  FROM drop_noncurrent
                UNION ALL
                SELECT removed_object, NULL FROM drop_object
                UNIon ALL
                SELECT removed_object, NULL FROM drop_del_mark
            ) as a
        )
        SELECT (removed_version).*
        FROM queues
    $quote$, i_bid, i_name, i_created, v_cid) INTO v_removed_version;

    IF NOT v_removed_version IS NULL AND v_removed_version.noncurrent IS NULL THEN
        EXECUTE format($quote$
            WITH prev as (
                SELECT created
                    FROM s3.objects_noncurrent
                    WHERE bid = %1$L /* i_bid */
                        AND name = %2$L /* i_name */
                    ORDER BY created DESC
                    LIMIT 1
            ), drop_noncurrent as (
                DELETE FROM s3.objects_noncurrent
                    WHERE bid =  %1$L /* i_bid */
                        AND name = %2$L /* i_name */
                        AND created in (select created from prev)
                RETURNING bid, name, created, data_size,
                    data_md5, mds_couple_id, mds_key_version, mds_key_uuid,
                    null_version, delete_marker, parts_count, parts, storage_class, creator_id,
                    metadata, acl, lock_settings
            ), insert_object as (
                INSERT INTO s3.objects(bid, name, created, data_size,
                    data_md5, mds_couple_id, mds_key_version,
                    mds_key_uuid, null_version, parts_count, parts, storage_class,
                    creator_id, metadata, acl, lock_settings)
                SELECT bid, name, created, data_size, data_md5, mds_couple_id,
                    mds_key_version, mds_key_uuid, null_version, parts_count, parts,
                    storage_class, creator_id, metadata, acl, lock_settings
                    FROM drop_noncurrent
                    WHERE NOT delete_marker
                RETURNING created
            ), insert_delete_marker as (
                INSERT INTO s3.object_delete_markers(bid, name, created, null_version, creator_id)
                SELECT bid, name, created, null_version, creator_id
                    FROM drop_noncurrent
                    WHERE delete_marker
                RETURNING created
            )
            SELECT 1
            FROM (
                SELECT * FROM insert_object
                UNION ALL
                SELECT * FROM insert_delete_marker
            ) as a
        $quote$, i_bid, i_name);
    END IF;

    IF NOT v_removed_version IS NULL THEN
        RETURN NEXT v_removed_version;
    END IF;
END;
$$;
