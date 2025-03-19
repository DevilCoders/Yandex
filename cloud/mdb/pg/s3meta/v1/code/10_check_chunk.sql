/*
 * Checks whether the specified chunk's available to perform the action.
 *
 * Args:
 * - i_chunk:
 *     An instance of ``code.chunk`` that respresents the chunk to check.
 * - i_write:
 *     Specifies the action to check (write or read).
 *
 * Raises:
 * - S3B04 (TemporarilyUnavailable):
 *     If the specified chunk is in read-only mode, while write action is
 *     checked.
 *     NOTE: This could happen during the chunk's resharding/migration, thus
 *     it's an internal error and is considered as a temporary unavailability
 *     issue.
 */
CREATE OR REPLACE FUNCTION v1_code.check_chunk_read_only(
    i_chunk v1_code.chunk,
    i_write boolean
) RETURNS void
LANGUAGE plpgsql STABLE AS $$
BEGIN
    IF i_write AND i_chunk.read_only THEN
        RAISE EXCEPTION 'Bucket chunk is temporarily unavailable'
            USING ERRCODE = 'S3B04';
    END IF;
END;
$$;
