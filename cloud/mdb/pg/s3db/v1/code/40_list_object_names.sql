/*
 * Returns some or all (up to ``i_limit``) names of the objects or common prefixes in a bucket's chunk.
 *
 * Args:
 * - i_bid:
 *    ID of the bucket to list objects from.
 * - i_prefix:
 *    Limits the response to keys that begin with the specified prefix.
 * - i_delimiter:
 *    A single character used to separate groups of keys.
 * - i_start_after:
 *    Specifies the key (or a common prefix) to start listing after.
 * - i_limit:
 *    Sets the maximum number of keys returned in the response.
 *
 * Returns:
 *   List of the bucket chunk's keys that satisfy the search criteria specified
 *   by ``i_prefix``, ``i_delimiter``, ``i_start_after``. Each common prefix
 *   (or a "directory") will be marked by ``is_prefix`` field set to True.
 *   This function skips delete markers for buckets with enabled versioning.
 *
 */
CREATE OR REPLACE FUNCTION v1_code.list_object_names(
    i_bid uuid,
    i_prefix text,
    i_delimiter text,
    i_start_after text,
    i_limit integer
) RETURNS SETOF v1_code.list_name_item
LANGUAGE plpgsql STABLE AS $function$
DECLARE
    v_prefix text;
    v_start_after text;
    v_escaped_prefix text;
    v_start_marker text;
    v_after_delimiter text;
BEGIN
    IF char_length(coalesce(i_delimiter, '')) != 1
    THEN
        RAISE EXCEPTION 'Delimiter shoud be exactly one character'
            USING ERRCODE = '22023';
    END IF;

    v_prefix := coalesce(i_prefix, '');
    v_start_after := coalesce(i_start_after, '');

    /* NOTE: delimiter should be single character and we supposed collation 'C' for names. */
    v_after_delimiter := chr(ascii(i_delimiter) + 1);
    v_escaped_prefix := regexp_replace(v_prefix, '([_%\\])', '\\\&', 'g');

    IF (v_start_after = v1_impl.get_key_common_prefix(v_start_after, v_prefix, i_delimiter))
    THEN
        v_start_marker := left(v_start_after, char_length(v_start_after) - 1) || v_after_delimiter;
    ELSE
        v_start_marker := v_start_after || chr(1);
    END IF;

    RETURN QUERY
        EXECUTE format($quote$
            WITH RECURSIVE objects AS (
                SELECT * FROM (
                    SELECT
                    v1_impl.get_name_listing_item(
                        s3o.bid, s3o.name,
                        %2$L, %3$L, %7$L, /*v_prefix, i_delimiter, v_after_delimiter*/
                        1
                    ) AS listing_item
                    FROM s3.objects s3o
                    WHERE bid = %1$L /*i_bid*/
                        AND name >= %4$L /*v_start_marker*/
                        AND name LIKE %6$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                        AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                    ORDER BY name ASC
                    LIMIT 1
                ) a /*subquery because we need ORDER BY name ASC LIMIT 1 with UNION ALL*/
                UNION ALL
                SELECT
                    (SELECT
                        v1_impl.get_name_listing_item(
                            s3o.bid, s3o.name,
                            %2$L, %3$L, %7$L, /*v_prefix, i_delimiter, v_after_delimiter*/
                            (objects.listing_item).level + 1
                        )
                        FROM s3.objects s3o
                        WHERE bid = %1$L /*i_bid*/
                            AND name >= (objects.listing_item).start_marker
                            AND name LIKE %6$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                            AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                        ORDER BY name LIMIT 1
                    ) AS listing_item
                FROM objects
                WHERE (objects.listing_item).name IS NOT NULL
                    AND (objects.listing_item).level < %5$L /*i_limit*/
            )
            SELECT (objects.listing_item).bid,
                (objects.listing_item).name,
                (objects.listing_item).is_prefix,
                (objects.listing_item).level as idx
            FROM objects
            WHERE (objects.listing_item).level IS NOT NULL
        $quote$, i_bid, v_prefix, i_delimiter, v_start_marker,
                i_limit, v_escaped_prefix, v_after_delimiter);
END;
$function$;
