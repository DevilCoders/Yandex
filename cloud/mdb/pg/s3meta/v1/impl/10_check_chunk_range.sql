/*
 * Checks whether the chunk's range could contain a key with the specified
 * restrictions.
 *
 * Args:
 * - i_start_key, i_end_key:
 *     Specify "left" and "right" boundaries of the chunk's range.
 * - i_prefix:
 *     A prefix that an appropriate key should start with.
 * - i_start_after:
 *     A key that an appropriate key should be greater than.
 *
 * Returns:
 *   TRUE or FALSE if the chunk's range could possibly contain a key that
 *   conforms to the specified restrictions.
 */
CREATE OR REPLACE FUNCTION v1_impl.check_chunk_range(
    i_start_key text,
    i_end_key text,
    i_prefix text,
    i_start_after text
) RETURNS boolean
IMMUTABLE
LANGUAGE plpgsql AS $$
BEGIN
    IF (i_prefix IS NOT NULL) THEN
        /*
         * If `i_prefix` is specified and the chunk's "left" boundary is greater
         * than the `i_prefix`, no keys of the chunk could start with the prefix.
         *
         * NOTE: It's not enough to simply check ``i_start_key > i_prefix``, as
         * `i_start_key` could start with the `i_prefix` and is greater than the
         * `i_prefix`, i.e. ("abc" > "ab"). To ensure no keys of the chunk start
         * with the `i_prefix` the "left" boundary's prefix of the same length
         * should be greater than the `i_prefix`.
         */
        IF (i_start_key IS NOT NULL
            AND left(i_start_key, char_length(i_prefix)) > i_prefix)
        THEN
            RETURN FALSE;
        END IF;

        /*
         * If `i_prefix` is specified and the chunk's "right" boundary is less
         * than the `i_prefix`, no keys of the chunk could start with the prefix.
         *
         * NOTE: It's not enough to simply check ``i_end_key < i_prefix`` as
         * with `i_start_key` (see above).
         */
        IF (i_end_key IS NOT NULL
            AND left(i_end_key, char_length(i_prefix)) < i_prefix)
        THEN
            RETURN FALSE;
        END IF;
    END IF;

    /*
     * If `i_start_after` is specified and it's greater than the "right" boundary
     * of the chunk, no keys of the chunk could be greater than the `i_start_after`.
     */
    IF (i_start_after IS NOT NULL
        AND i_end_key IS NOT NULL
        AND i_end_key < i_start_after)
    THEN
        RETURN FALSE;
    END IF;

    RETURN TRUE;
END;
$$;
