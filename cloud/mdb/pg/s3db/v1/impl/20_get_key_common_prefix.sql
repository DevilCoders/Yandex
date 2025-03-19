/*
 * Returns the key's "common prefix".
 * "Common prefix" is a part of the key that starts with the specified prefix
 * up to the first occurence of the specified delimiter (including the delimiter).
 *
 * Args:
 * - i_name:
 *     The key's name.
 * - i_prefix:
 *     Prefix the key should start with.
 * - i_delimiter:
 *     A character (or a string) that should be looked for in the key after the
 *     prefix.
 *
 * Returns:
 *   The smallest key's prefix that starts with the `i_prefix` and ends with the
 *   `i_delimiter` if there's is one, or NULL otherwise.
 */
CREATE OR REPLACE FUNCTION v1_impl.get_key_common_prefix(
    i_name text,
    i_prefix text,
    i_delimiter text
) RETURNS text
IMMUTABLE
LANGUAGE plpgsql AS $$
DECLARE
    v_prefix text;
    -- The key's suffix after the prefix
    v_after_prefix text;
    -- Position of the delimiter in the key's suffix after the prefix
    v_delimiter_pos integer;
    -- Length of the key's common prefix
    v_common_prefix_length integer;
BEGIN
    -- Use empty string as a prefix if `i_prefix` is not specified (i.e. NULL)
    v_prefix := coalesce(i_prefix, '');

    IF (left(i_name, char_length(i_prefix)) != i_prefix) THEN
        RETURN NULL;
    END IF;

    v_after_prefix := substring(i_name FROM char_length(i_prefix) + 1);
    v_delimiter_pos := position(i_delimiter IN v_after_prefix);

    IF (v_delimiter_pos > 0) THEN
    BEGIN
        -- Common prefix contains of the prefix, part of the key up to the delimiter
        -- and the delimiter itself
        v_common_prefix_length := char_length(i_prefix) + v_delimiter_pos - 1 + char_length(i_delimiter);
        RETURN substring(i_name FOR v_common_prefix_length);
    END;
    END IF;

    RETURN NULL;
END;
$$;
