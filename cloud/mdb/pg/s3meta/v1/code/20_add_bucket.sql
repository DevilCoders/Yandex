/*
 * Creates new bucket.
 *
 * Args:
 * - i_name:
 *     Name of the bucket to create.
 * - i_versioning:
 *     Versioning state of the bucket to create ('enabled', 'suspended',
 *     'disabled'). By default the created bucket will have 'disabled'
 *     versioning state.
 * - i_service_id:
 *     Service account's ID. By default NULL is used, but anonymous access
 *     for this operation is forbidden even if there's already such publicly
 *     available bucket.
 * - i_max_size:
 *     Maximal total size (in bytes) of objects in bucket.
 *     By default NULL is used, i.e. bucket have no such restriction.
 *
 * Raises:
 * - S3A01 (AccessDenied):
 *     Anonymous access is forbidden for this operation.
 * - S3B02 (BucketAlreadyExists):
 *     If bucket with the specified name already exists.
 * - S3B05 (BucketAlreadyOwnedByYou)
 *     If the specified bucket already exists and your service account
 *     already owns it.
 *
 * TODO: Remove ``i_versioning`` argument as S3 doesn't support bucket's
 * creation with versioning state other than 'disabled'.
 */
CREATE OR REPLACE FUNCTION v1_code.add_bucket(
    i_name text,
    i_versioning s3.bucket_versioning_type DEFAULT 'disabled',
    i_service_id bigint DEFAULT NULL,
    i_max_size bigint DEFAULT NULL,
    i_system_settings JSONB DEFAULT NULL
) RETURNS v1_code.bucket_chunk LANGUAGE plpgsql AS
$$
DECLARE
    res v1_code.bucket;
    result v1_code.bucket_chunk;
BEGIN
    /*
     * Anonymous access for this operation is forbidden, even if the requested
     * publicly available bucket already exists and S3B05 (Bucket already owned
     * by you) error is expected.
     */
    IF i_service_id IS NULL THEN
        RAISE EXCEPTION 'Anonymous access is forbidden'
            USING ERRCODE = 'S3A01';
    END IF;

    -- Bucket is created as "private" by default
    INSERT INTO s3.buckets (name, versioning, service_id, max_size,
                anonymous_read, anonymous_list, website_settings, cors_configuration,
                default_storage_class, yc_tags, lifecycle_rules, system_settings)
        VALUES (i_name, i_versioning, i_service_id, i_max_size, FALSE, FALSE,
         NULL, NULL, 0, NULL, NULL, i_system_settings)
        ON CONFLICT (name) DO NOTHING
        RETURNING bid, name, created, versioning, banned, service_id, max_size,
                anonymous_read, anonymous_list, website_settings, cors_configuration,
                default_storage_class, yc_tags, lifecycle_rules, system_settings, encryption_settings, state
            INTO res;

    -- ON CONFLICT DO NOTHING case
    IF NOT FOUND THEN
        res := v1_code.bucket_info(i_name);
        IF i_service_id IS NOT DISTINCT FROM res.service_id THEN
            RAISE EXCEPTION 'Bucket already owned by you'
                USING ERRCODE = 'S3B05';
        ELSE
            RAISE EXCEPTION 'Bucket with such name already exists'
                USING ERRCODE = 'S3B02';
        END IF;
    END IF;

    INSERT INTO s3.buckets_history (bid, name, created, deleted, service_id, yc_tags)
        VALUES (res.bid, res.name, res.created, '9999-12-31', res.service_id, res.yc_tags)
        ON CONFLICT (bid) DO UPDATE SET
            name=res.name,
            created=res.created,
            service_id=res.service_id,
            yc_tags=res.yc_tags;

    result.chunk := v1_code.create_chunk(res.bid);
    result.bucket := res;

    RETURN result;
END;
$$;
