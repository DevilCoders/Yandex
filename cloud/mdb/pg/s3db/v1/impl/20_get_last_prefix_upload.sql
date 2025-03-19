/*
 * Returns the key of the last multipart upload with the specified prefix.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket to search in.
 * - i_prefix:
 *     Prefix the last upload's key should start with. If this set to NULL the
 *     last key in the bucket is returned.
 *
 * Returns:
 *   Name of the last multipart upload that conforms to the specified
 *   restrictions or NULL if there's no such upload.
 */
CREATE OR REPLACE FUNCTION v1_impl.get_last_prefix_upload(
    i_bid uuid,
    i_prefix text
) RETURNS text
LANGUAGE plpgsql STABLE AS $$
DECLARE
    v_result text;
    v_escaped_prefix text;
BEGIN
    v_escaped_prefix := regexp_replace(i_prefix, '([_%\\])', '\\\&', 'g');
    /*
     * FIXME: Make this function as SQL STABLE instead of plpgsql.
     * Now pg_pathman have some troubles with execution of sql
     * functions. This will be fixed in one of next releases.
     */
    EXECUTE format($quote$
        SELECT name
            FROM s3.object_parts
            WHERE bid = %1$L
                AND (%2$L IS NULL OR
                    name LIKE %3$L || '%%' ESCAPE '\'
                    AND left(name, char_length(%2$L)) = %2$L
                )
            ORDER BY name DESC
            LIMIT 1
        $quote$, i_bid, i_prefix, v_escaped_prefix)
            INTO v_result;
    RETURN v_result;
END;
$$;
