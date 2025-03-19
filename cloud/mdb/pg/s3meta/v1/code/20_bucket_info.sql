/*
 * Returns the bucket's info.
 *
 * Args:
 * - i_name:
 *     Name of the bucket to get info about.
 *
 * Returns:
 *   An instance of ``code.bucket`` that represents the requested bucket.
 *
 * Raises:
 * - S3B01:
 *     If bucket with the specified name doesn't exist.
 */
CREATE OR REPLACE FUNCTION v1_code.bucket_info(
    i_name text
) RETURNS v1_code.bucket
LANGUAGE plpgsql STABLE AS
$$
DECLARE
    res v1_code.bucket;
BEGIN
    SELECT bid, name, created, versioning, banned, service_id, max_size,
            anonymous_read, anonymous_list, website_settings, cors_configuration,
            default_storage_class, yc_tags, lifecycle_rules, system_settings, encryption_settings, state
        INTO res
        FROM s3.buckets
        WHERE name = i_name
        LIMIT 1;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such bucket'
            USING ERRCODE = 'S3B01';
    END IF;

    RETURN res;
END;
$$;
