/*
 * Updates the bucket's info.
 *
 * Args:
 * - i_name:
 *     Name of the bucket to update info about.
 * - i_versioning:
 *     Versioning state of the bucket to set, if not NULL.
 * - i_banned:
 *     Ban (or unban) the bucket, if not NULL.
 * - i_max_size:
 *     Set maximal total size (in bytes) of objects in bucket.
 *
 * Raises:
 * - 22004:
 *     If all requested bucket's properties are not specified (i.e. set to NULL).
 *     If requested bucket's versioning state could not be set (i.e. bucket's
 *     versioning was 'enabled' earlier and could not be returned to 'disabled').
 * - S3B01:
 *     If the specified bucket doesn't exist.
 *
 * TODO: Add support for `i_service_id` to limit access to service accounts'
 * buckets.
 */
CREATE OR REPLACE FUNCTION v1_code.modify_bucket(
    i_name text,
    i_versioning s3.bucket_versioning_type,
    i_banned boolean,
    i_max_size bigint DEFAULT NULL,
    i_anonymous_read boolean DEFAULT NULL,
    i_anonymous_list boolean DEFAULT NULL,
    i_website_settings JSONB DEFAULT NULL,
    i_cors_configuration JSONB DEFAULT NULL,
    i_default_storage_class int DEFAULT NULL,
    i_yc_tags JSONB DEFAULT NULL,
    i_lifecycle_rules JSONB DEFAULT NULL,
    i_system_settings JSONB DEFAULT NULL,
    i_service_id bigint DEFAULT NULL,
    i_encryption_settings JSONB DEFAULT NULL
) RETURNS v1_code.bucket LANGUAGE plpgsql AS
$$
DECLARE
    bucket_id uuid;
    v_versioning s3.bucket_versioning_type;
    v_banned boolean;
    v_res v1_code.bucket;
    v_state s3.bucket_state;
BEGIN
    IF i_versioning IS NULL AND i_banned IS NULL AND i_max_size IS NULL
        AND i_anonymous_read IS NULL AND i_anonymous_list IS NULL
        AND i_website_settings IS NULL AND i_cors_configuration IS NULL
        AND i_default_storage_class IS NULL AND i_yc_tags IS NULL
        AND i_lifecycle_rules IS NULL AND i_system_settings IS NULL
        AND i_service_id IS NULL AND i_encryption_settings IS NULL
    THEN
        RAISE EXCEPTION 'Invalid modify_bucket(name = %) arguments, all are NULL', i_name
                        USING ERRCODE = '22004';
    END IF;

    SELECT bid, versioning, banned, state
        INTO bucket_id, v_versioning, v_banned, v_state
        FROM s3.buckets
        WHERE name = i_name;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such bucket'
            USING ERRCODE = 'S3B01';
    ELSIF v_versioning IN ('enabled', 'suspended')
        AND i_versioning = 'disabled'
    THEN
        RAISE EXCEPTION 'Bucket versioning could not be changed from enabled '
                        'to disabled.'
            USING ERRCODE = '22004';
    ELSIF v_state = 'deleting' THEN
        RAISE EXCEPTION 'Bucket is in deleting state'
            USING ERRCODE = 'S3B03';
    END IF;

    IF i_lifecycle_rules IS NOT NULL THEN
        PERFORM v1_code.add_enabled_lifecycle_rules(bucket_id, i_lifecycle_rules);
    END IF;

    UPDATE s3.buckets
        SET versioning = coalesce(i_versioning, versioning),
            banned = coalesce(i_banned, banned),
            max_size = coalesce(i_max_size, max_size),
            anonymous_read = coalesce(i_anonymous_read, anonymous_read),
            anonymous_list = coalesce(i_anonymous_list, anonymous_list),
            website_settings = coalesce(i_website_settings, website_settings),
            cors_configuration = coalesce(i_cors_configuration, cors_configuration),
            default_storage_class = coalesce(i_default_storage_class, default_storage_class),
            yc_tags = coalesce(i_yc_tags, yc_tags),
            lifecycle_rules = coalesce(i_lifecycle_rules, lifecycle_rules),
            system_settings = coalesce(i_system_settings, system_settings),
            service_id = coalesce(i_service_id, service_id),
            encryption_settings = coalesce(i_encryption_settings, encryption_settings)
        WHERE bid = bucket_id
        RETURNING bid, name, created, versioning, banned, service_id, max_size,
                anonymous_read, anonymous_list, website_settings, cors_configuration,
                default_storage_class, yc_tags, lifecycle_rules, system_settings,
                encryption_settings, state
            INTO v_res;

    UPDATE s3.buckets_history
        SET yc_tags = v_res.yc_tags,
            service_id = v_res.service_id
        WHERE bid = bucket_id;

    RETURN v_res;

END;
$$;

