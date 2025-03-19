/*
 * Returns all chunks of the bucket that could possible contain any key that
 * conforms to the restrictions.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket.
 * - i_prefix:
 *     A prefix that an appropriate key should start with.
 * - i_start_after:
 *     A key that an appropriate key should be greater than.
 *
 * Raises:
 * - S3B01:
 *     If the specified bucket doesn't exist.
 * - S3B03:
 *     If the specified bucket was banned and no requests could be made to it.
 */
CREATE OR REPLACE FUNCTION v1_code.get_bucket_chunks(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text,
    i_start_after text
) RETURNS SETOF v1_code.chunk
LANGUAGE plpgsql AS $$
DECLARE
    v_bucket v1_code.bucket;
BEGIN
    SELECT bid, name, created, versioning, banned, service_id, max_size
        INTO v_bucket
        FROM s3.buckets
        WHERE name = i_bucket_name
          AND bid = i_bid;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such bucket'
            USING ERRCODE = 'S3B01';
    ELSIF v_bucket.banned THEN
        RAISE EXCEPTION 'Access denied to the bucket'
            USING ERRCODE = 'S3B03';
    END IF;

    RETURN QUERY
        SELECT bid, cid, created, read_only, start_key, end_key, shard_id
            FROM s3.chunks
            WHERE bid = v_bucket.bid
              AND v1_impl.check_chunk_range(start_key, end_key, i_prefix, i_start_after)
            ORDER BY start_key ASC NULLS FIRST;
END;
$$;
