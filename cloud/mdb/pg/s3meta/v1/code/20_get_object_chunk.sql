/*
 * Returns bucket's chunk the object is expected to reside in.
 *
 * Args:
 * - i_bucket_name:
 *     Name of the bucket.
 * - i_name:
 *     Name of the object.
 * - i_write:
 *     Specifies whether the object will be written to or read from the requested
 *     chunk.
 *
 * Returns:
 *   An instance of ``code.chunk``.
 *
 * Raises:
 * - S3B01:
 *     If the requested bucket doesn't exist.
 * - S3B03:
 *     If the requested bucket is banned.
 * - 53400:
 *     If the requested bucket is in read-only state while ``i_write`` is set to true.
 *     If the requested chunk is in read-only state while ``i_write`` is set to true.
 * - 39004:
 *     If no appropriate chunk was found.
 */
CREATE OR REPLACE FUNCTION v1_code.get_object_chunk(
    i_bucket_name text,
    i_name text,
    i_write boolean DEFAULT false
) RETURNS v1_code.chunk
LANGUAGE plpgsql STABLE AS $$
DECLARE
    v_bucket v1_code.bucket;
    v_chunk v1_code.chunk;
BEGIN
    SELECT bid, name, created, versioning, banned, service_id, max_size
        INTO v_bucket
        FROM s3.buckets
        WHERE name = i_bucket_name;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such bucket'
            USING ERRCODE = 'S3B01';
    ELSIF v_bucket.banned THEN
        RAISE EXCEPTION 'Access denied to the bucket'
            USING ERRCODE = 'S3B03';
    END IF;

    SELECT * INTO v_chunk
        FROM s3.chunks
        WHERE bid = v_bucket.bid
            AND coalesce(start_key, '') <= i_name
    ORDER BY coalesce(start_key, '') DESC
    LIMIT 1;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Could not get info about chunk for object % '
                        'in bucket %', i_name, i_bucket_name
            USING ERRCODE = '39004';
    END IF;

    PERFORM v1_code.check_chunk_read_only(v_chunk, i_write);
    RETURN v_chunk;
END;
$$;
