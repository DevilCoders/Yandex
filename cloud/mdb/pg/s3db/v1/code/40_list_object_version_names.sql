/*
 * Returns some or all (up to ``i_limit``) names of the current object versions or
 * common prefixes in a bucket's chunk including delete marks.
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
 * - i_start_after_created:
 *    Specifies the version creation date (or null for a common prefix) to start listing after.
 * - i_limit:
 *    Sets the maximum number of keys returned in the response.
 *
 * Returns:
 *   List of the bucket chunk's keys that satisfy the search criteria specified
 *   by ``i_prefix``, ``i_delimiter``, ``i_start_after``. Each common prefix
 *   (or a "directory") will be marked by ``is_prefix`` field setted to True.
 *
 */
CREATE OR REPLACE FUNCTION v1_code.list_object_version_names(
    i_bid uuid,
    i_prefix text,
    i_delimiter text,
    i_start_after text,
    i_start_after_created timestamp with time zone,
    i_limit integer
) RETURNS SETOF v1_code.list_name_item
LANGUAGE plpgsql STABLE AS $function$
DECLARE
    v_prefix text;
    v_start_after text;
    v_escaped_prefix text;
    v_start_marker text;
    v_start_after_created timestamp with time zone;
    v_after_delimiter text;
BEGIN
    IF char_length(coalesce(i_delimiter, '')) != 1
    THEN
        RAISE EXCEPTION 'Delimiter shoud be exactly one character'
            USING ERRCODE = '22023';
    END IF;

    v_prefix := coalesce(i_prefix, '');
    v_start_after := coalesce(i_start_after, '');
    v_start_after_created := i_start_after_created;

    /* NOTE: delimiter should be single character and we supposed collation 'C' for names. */
    v_after_delimiter := chr(ascii(i_delimiter) + 1);
    v_escaped_prefix := regexp_replace(v_prefix, '([_%\\])', '\\\&', 'g');

    IF (v_start_after = v1_impl.get_key_common_prefix(v_start_after, v_prefix, i_delimiter))
    THEN
        v_start_marker := left(v_start_after, char_length(v_start_after) - 1) || v_after_delimiter;
        v_start_after_created := NULL;
    ELSE
        v_start_marker := v_start_after||chr(1);
    END IF;

    RETURN QUERY
        EXECUTE format($quote$
            WITH RECURSIVE delete_markers AS (
                SELECT * FROM (
                    SELECT
                    v1_impl.get_name_listing_item(
                        s3o.bid, s3o.name,
                        %2$L, %3$L, %8$L, /*v_prefix, i_delimiter, v_after_delimiter*/
                        1
                    ) AS listing_item
                    FROM s3.object_delete_markers s3o
                    WHERE bid = %1$L /*i_bid*/
                        AND name LIKE %7$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                        AND name >= %9$L /*v_start_after*/
                        AND (name >= %4$L /*v_start_marker*/
                            OR (%5$L IS NOT NULL /*v_start_after_created*/
                                AND name = %9$L) /*v_start_after*/
                        )
                    ORDER BY name ASC
                    LIMIT 1
                ) a /*subquery because we need ORDER BY name ASC LIMIT 1 with UNION ALL*/
                UNION ALL
                SELECT
                    (SELECT
                        v1_impl.get_name_listing_item(
                            s3o.bid, s3o.name,
                            %2$L, %3$L, %8$L, /*v_prefix, i_delimiter, v_after_delimiter*/
                            (delete_markers.listing_item).level + 1
                        )
                        FROM s3.object_delete_markers s3o
                        WHERE bid = %1$L /*i_bid*/
                            AND name >= (delete_markers.listing_item).start_marker
                            AND name LIKE %7$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                            AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                        ORDER BY name LIMIT 1
                    ) AS listing_item
                FROM delete_markers
                WHERE (delete_markers.listing_item).name IS NOT NULL
                    AND (delete_markers.listing_item).level < %6$L /*i_limit*/
            ), objects AS (
                SELECT * FROM (
                    SELECT
                    v1_impl.get_name_listing_item(
                        s3o.bid, s3o.name,
                        %2$L, %3$L, %8$L, /*v_prefix, i_delimiter, v_after_delimiter*/
                        1
                    ) AS listing_item
                    FROM s3.objects s3o
                    WHERE bid = %1$L /*i_bid*/
                        AND name LIKE %7$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                        AND name >= %9$L /*v_start_after*/
                        AND (name >= %4$L /*v_start_marker*/
                            OR (%5$L IS NOT NULL /*v_start_after_created*/
                                AND name = %9$L) /*v_start_after*/
                        )
                    ORDER BY name ASC
                    LIMIT 1
                ) a /*subquery because we need ORDER BY name ASC LIMIT 1 with UNION ALL*/
                UNION ALL
                SELECT
                    (SELECT
                        v1_impl.get_name_listing_item(
                            s3o.bid, s3o.name,
                            %2$L, %3$L, %8$L, /*v_prefix, i_delimiter, v_after_delimiter*/
                            (objects.listing_item).level + 1
                        )
                        FROM s3.objects s3o
                        WHERE bid = %1$L /*i_bid*/
                            AND name >= (objects.listing_item).start_marker
                            AND name LIKE %7$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                            AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                        ORDER BY name LIMIT 1
                    ) AS listing_item
                FROM objects
                WHERE (objects.listing_item).name IS NOT NULL
                    AND (objects.listing_item).level < %6$L /*i_limit*/
            )
            SELECT bid, name, is_prefix, cast(row_number() over(order by name COLLATE "C") as int) as idx
            FROM (
                SELECT (delete_markers.listing_item).bid,
                    (delete_markers.listing_item).name,
                    (delete_markers.listing_item).is_prefix
                FROM delete_markers
                WHERE (delete_markers.listing_item).level IS NOT NULL
                UNION
                SELECT (objects.listing_item).bid,
                    (objects.listing_item).name,
                    (objects.listing_item).is_prefix
                FROM objects
                WHERE (objects.listing_item).level IS NOT NULL
            ) as i
            ORDER BY idx
            LIMIT %6$L + 1 /*i_limit*/ /* there is case when first key will be without versions after additional filtration*/
        $quote$, i_bid, v_prefix, i_delimiter, v_start_marker, i_start_after_created,
                i_limit, v_escaped_prefix, v_after_delimiter, v_start_after);
END;
$function$;
