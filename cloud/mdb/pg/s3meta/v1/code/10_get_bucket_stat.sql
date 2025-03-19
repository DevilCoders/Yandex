/*
 * DEPRECATED
 * Returns statistics of concrete bucket.
 *
 * Args:
 * - i_bucket_name:
 *     Name of the bucket.
 *
 */
CREATE OR REPLACE FUNCTION v1_code.get_bucket_stat(
    i_bucket_name text
) RETURNS v1_code.bucket_stat
LANGUAGE plpgsql STABLE AS
$$
DECLARE
    res v1_code.bucket_stat;
BEGIN
    SELECT
        bid,
        name,
        service_id,
        CAST(0 AS INT) as storage_class,
        chunks_count,
        simple_objects_count,
        simple_objects_size,
        multipart_objects_count,
        multipart_objects_size,
        objects_parts_count,
        objects_parts_size,
        updated_ts,
        max_size
    INTO res
    FROM s3.bucket_stat
    WHERE name = i_bucket_name;

    IF NOT FOUND
    THEN
        RAISE EXCEPTION 'No such bucket'
        USING ERRCODE = 'S3B01';
    END IF;

    RETURN res;
END;
$$;
