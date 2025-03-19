/* This function check created of head object versions
   to avoid collisions in s3.objects_noncurrent.
*/
CREATE OR REPLACE FUNCTION v1_impl.check_old_transaction(
    i_versioning s3.bucket_versioning_type,
    i_created timestamp with time zone
) RETURNS void
LANGUAGE plpgsql AS $$
BEGIN
    IF (i_versioning = 'enabled' OR i_versioning = 'suspended')
           AND i_created >= current_timestamp THEN
        RAISE EXCEPTION 'Current transaction is too old'
            USING ERRCODE = 'S3K04';
END IF;
END;
$$;
