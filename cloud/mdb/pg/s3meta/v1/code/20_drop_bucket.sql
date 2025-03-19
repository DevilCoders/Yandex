/*
 * Deletes the bucket.
 *
 * Args:
 * - i_name:
 *     Name of the bucket to delete.
 *
 * Raises:
 * - S3B01:
 *     If bucket with the specified name doesn't exist.
 *
 * TODO: Right now we don't check that bucket has no objects, but according to
 * S3 API all objects (including all object versions and delete markers) in the
 * bucket must be deleted before the bucket itself can be deleted.
 */
CREATE OR REPLACE FUNCTION v1_code.drop_bucket(
    i_name text
) RETURNS void LANGUAGE plpgsql AS
$$
DECLARE
    bucket_id uuid;
BEGIN
    SELECT bid INTO bucket_id
        FROM s3.buckets
        WHERE name = i_name;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such bucket'
            USING ERRCODE = 'S3B01';
    END IF;

    WITH chunks AS (
        DELETE FROM s3.chunks
            WHERE bid = bucket_id
        RETURNING bid, cid, created,
                  start_key, end_key,
                  shard_id
    )
    INSERT INTO s3.chunks_delete_queue (
            bid, cid, bucket_name,
            created, start_key,
            end_key, shard_id
        )
        SELECT bid, cid, i_name,
               created, start_key,
               end_key, shard_id
        FROM chunks;

    DELETE FROM s3.buckets WHERE bid = bucket_id;
    UPDATE s3.buckets_history SET deleted = current_timestamp WHERE bid = bucket_id;
END;
$$;
