/*
 * Adds new object (or a version) to the bucket.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket to add the object to.
 * - i_versioning:
 *     Versioning state of the bucket.
 * - i_name:
 *     Name of the object to add.
 * - i_cid:
 *     ID of the bucket's chunk to which the object belongs. Does not used and should be removed.
 * - i_data_size, i_data_md5:
 *     Size and MD5 hash of the object's data.
 * - i_mds_couple_id, i_mds_key_version, i_mds_key_uuid:
 *     MDS storage information of the stored object's data.
 * - i_parts:
 *     List of the object's data parts if it was multipart uploaded, or NULL if
 *     the object was uploaded as a single part (i.e. simple upload).
 * - i_storage_class:
 *     Object's storage class.
 * - i_creator_id:
 *     Requester ID.
 * - i_metadata:
 *     Object's S3 metadata.
 * - i_acl:
 *     Object's ACL.
 * - i_separate_parts:
 *     When adding a multipart object, store parts in `completed_parts` table
 *     instead of the inline `parts` array
 * - i_lock_settings:
 *     Object lock settings.
 *
 * Returns:
 *   An instance of ``code.object`` that represents added object.
 *
 * NOTE: If the bucket's versioning isn't 'enabled' previous 'null' version of
 * the object will be removed, if any.
 */
CREATE OR REPLACE FUNCTION v1_code.add_object(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_cid bigint,
    i_data_size bigint,
    i_data_md5 uuid,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_parts s3.object_part[] DEFAULT NULL,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_separate_parts boolean DEFAULT FALSE,
    i_lock_settings JSONB DEFAULT NULL
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
DECLARE
    v_cid bigint;
    v_retry_object v1_code.object;
    v_current_version v1_code.object;
    v_created_object v1_code.object;
BEGIN
    /*
     * The single part object is limited by 5 gigabytes. Check its size here to
     * raise appropriate exception instead of generic ``check_violation`` error.
     */
    IF i_parts IS NULL AND i_data_size > 5368709120 THEN
        RAISE EXCEPTION 'Object is too large'
            USING ERRCODE = 'S3K03';
    END IF;

    /*
     * S3 allows the last part to be empty
     * But our offset index doesn't allow empty parts in the DB. So remove it
     */
    IF i_parts IS NOT NULL AND i_parts[array_length(i_parts, 1)].data_size = 0 AND
        (i_separate_parts OR array_length(i_parts, 1) > 1) THEN
        IF i_parts[array_length(i_parts, 1)].mds_key_uuid IS NOT NULL THEN
            RAISE EXCEPTION 'Empty part should have empty MDS ID';
        END IF;
        i_parts := i_parts[1 : array_length(i_parts, 1)-1];
    END IF;

    /*
     * Find actual object chunk, block this to share and return this chunk id as object's cid.
     * NOTE: we actually store NULL in object's cid due to possibly splitting and moving chunks.
     */
    v_cid := v1_code.get_object_cid(i_bid, i_name);
    PERFORM v1_impl.lock_object(i_bid, i_name);

    IF i_parts IS NULL AND (
        i_mds_couple_id IS NOT NULL OR
        i_mds_key_version IS NOT NULL OR
        i_mds_key_uuid IS NOT NULL
    ) THEN
        /*
        * Check for bad retry with same mds key.
        */
        EXECUTE format($quote$
            SELECT bid, name, created, %3$L /* v_cid */, data_size, data_md5, mds_namespace, mds_couple_id,
                mds_key_version, mds_key_uuid, null_version, delete_marker,
                parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
            FROM s3.objects
            WHERE bid = %1$L /* i_bid */
                AND name = %2$L /* i_name */
                AND parts is NULL
            FOR UPDATE
        $quote$, i_bid, i_name, v_cid) INTO v_retry_object;

        IF NOT v_retry_object IS NULL
                AND v_retry_object.data_size = i_data_size
                AND v_retry_object.data_md5 = i_data_md5
                AND v_retry_object.mds_couple_id IS NOT DISTINCT FROM i_mds_couple_id
                AND v_retry_object.mds_key_version IS NOT DISTINCT FROM i_mds_key_version
                AND v_retry_object.mds_key_uuid IS NOT DISTINCT FROM i_mds_key_uuid
                AND v_retry_object.storage_class IS NOT DISTINCT FROM i_storage_class
                THEN
            RETURN v_retry_object;
        END IF;
    END IF;

    v_current_version := v1_impl.drop_object_current(i_bid, v_cid, i_name, NULL);

    IF NOT v_current_version IS NULL THEN
        PERFORM v1_impl.check_old_transaction(i_versioning, (v_current_version).created);

        IF i_versioning = 'enabled' THEN
            PERFORM v1_impl.push_object_noncurrent(v_current_version);
        ELSE
            IF i_versioning = 'disabled' OR i_versioning = 'suspended' AND v_current_version.null_version THEN
                -- Delete parts of the current version because we delete the version itself
                PERFORM v1_impl.drop_object_parts(v_current_version);
                PERFORM v1_impl.storage_delete_queue_push(v_current_version);
                PERFORM v1_code.chunks_counters_queue_push(
                    OPERATOR(v1_code.-)v1_code.object_get_chunk_counters(v_current_version));
            ELSE
                -- Leave parts as is because we only move current version to noncurrent
                PERFORM v1_impl.remove_null_object_noncurrent(i_bid, v_cid, i_name);
                PERFORM v1_impl.push_object_noncurrent(v_current_version);
            END IF;
        END IF;
    END IF;

    EXECUTE format($quote$
        INSERT INTO s3.objects (
            bid, name, created, cid, data_size,
            data_md5, mds_namespace, mds_couple_id, mds_key_version,
            mds_key_uuid, null_version, delete_marker,
            parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings)
        VALUES (%1$L /* i_bid */, %2$L /* i_name */, current_timestamp, NULL,
            %3$L /* i_data_size */, %4$L /* i_data_md5 */, %5$L /* i_mds_namespace */, %6$L /* i_mds_couple_id */,
            %7$L /* i_mds_key_version */, %8$L /* i_mds_key_uuid */, %9$L /* i_versioning */ != 'enabled',
            false, %10$L /* array_length(i_parts, 1)*/, %11$L /* i_parts */,
            %12$L /* i_storage_class */, %13$L /* i_creator_id */, %14$L /* i_metadata */, %15$L /* i_acl */,
            %16$L /* lock_settings */)
        RETURNING bid, name, created, %17$L /* v_cid */, data_size,
            data_md5, mds_namespace, mds_couple_id, mds_key_version,
            mds_key_uuid, null_version, delete_marker,
            parts_count, parts, storage_class, creator_id, metadata, acl, lock_settings
    $quote$, i_bid, i_name, i_data_size, i_data_md5, i_mds_namespace, i_mds_couple_id, i_mds_key_version,
        i_mds_key_uuid, i_versioning, array_length(i_parts, 1),
        CASE WHEN i_separate_parts THEN NULL ELSE i_parts END,
        i_storage_class, i_creator_id, i_metadata, i_acl, i_lock_settings, v_cid
    ) INTO v_created_object;

    IF i_separate_parts AND i_parts IS NOT NULL THEN
        EXECUTE format($quote$
            WITH parts AS (
                SELECT (unnest(%1$L :: s3.object_part[] /* i_parts */)).*
            ),
            parts_enc AS (
                SELECT *
                FROM jsonb_array_elements_text((%5$L /* i_metadata */)::jsonb->'encryption'->'parts_meta')
                WITH ORDINALITY AS t (encryption, part_id)
            ),
            parts_ext AS (
                SELECT
                    parts.*,
                    (SUM(parts.data_size) OVER (ORDER BY parts.part_id)) AS end_offset,
                    parts_enc.encryption
                FROM parts
                LEFT JOIN parts_enc ON parts_enc.part_id=parts.part_id
            )
            INSERT INTO s3.completed_parts (bid, name, object_created, part_id, end_offset,
                created, data_size, data_md5, mds_couple_id, mds_key_version,
                mds_key_uuid, storage_class, encryption)
            SELECT %2$L /* i_bid */, %3$L /* i_name */, %4$L /* v_created_object.created */,
                part_id, end_offset, created, data_size, data_md5, mds_couple_id, mds_key_version,
                mds_key_uuid, %6$L /* v_created_object.storage_class */, encryption
            FROM parts_ext
        $quote$, i_parts, i_bid, i_name, v_created_object.created, i_metadata, v_created_object.storage_class);
    END IF;

    PERFORM v1_code.chunks_counters_queue_push(v1_code.object_get_chunk_counters(v_created_object));
    RETURN v_created_object;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.add_object(
    i_bucket_name text,
    i_bid uuid,
    i_versioning s3.bucket_versioning_type,
    i_name text,
    i_cid bigint,
    i_data_size bigint,
    i_data_md5 uuid,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_parts s3.object_part[] DEFAULT NULL,
    i_storage_class int DEFAULT NULL,
    i_creator_id text DEFAULT NULL,
    i_metadata JSONB DEFAULT NULL,
    i_acl JSONB DEFAULT NULL,
    i_separate_parts boolean DEFAULT FALSE,
    i_lock_settings JSONB DEFAULT NULL
) RETURNS v1_code.object
LANGUAGE plpgsql AS $$
BEGIN
    RETURN v1_code.add_object(
        i_bucket_name, i_bid, i_versioning, i_name, i_cid, i_data_size, i_data_md5,
        /*i_mds_namespace*/ NULL, i_mds_couple_id, i_mds_key_version, i_mds_key_uuid,
        i_parts, i_storage_class, i_creator_id, i_metadata, i_acl, i_separate_parts,
        i_lock_settings);
END;
$$;
