/*
 * Returns some or all (up to ``i_limit``) of the objects in a bucket's chunk.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *    Name and ID of the bucket to list objects from.
 * - i_prefix:
 *    Limits the response to keys that begin with the specified prefix.
 * - i_delimiter:
 *    A single character (or group of characters) used to group keys.
 *    If ``i_prefix`` is specified, all keys that contain the same string between
 *    the ``i_prefix`` and the first occurrence of the ``i_delimiter`` after the
 *    ``i_prefix`` are grouped under a single result element. If ``i_prefix`` is
 *    not specified (i.e. NULL), the substring starts at the beginning of the key.
 *    All of the keys rolled up in a common prefix count as a single return when
 *    calculating the number of returns.
 *    Returned instance of ``code.object`` will have all attributes NULL-ed, but
 *    ``name`` that will contain a common prefix.
 * - i_start_after:
 *    Specifies the key (or a common prefix) to start listing after.
 * - i_limit:
 *    Sets the maximum number of keys returned in the response.
 *
 * Returns:
 *   List of the bucket chunk's keys that satisfy the search criteria specified
 *   by ``i_prefix``, ``i_delimiter``, ``i_start_after``. Each common prefix
 *   (or a "directory") will be represented as an instance of ``code.object``
 *   with all attributes but name set to NULL.
 *
 * TODO:
 * - Should a common prefix be returned if all of the keys with the common prefix
 *   have delete markers as their current version?
 */
CREATE OR REPLACE FUNCTION v1_code.list_objects(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text,
    i_delimiter text,
    i_start_after text,
    i_limit integer DEFAULT 1000
) RETURNS SETOF v1_code.object
LANGUAGE plpgsql STABLE AS $function$
DECLARE
    v_prefix text;
    v_start_after text;
    v_delimiter_position integer;
    v_current_object v1_code.object;
    v_escaped_prefix text;
BEGIN
    /*
     * The function works as follows. If delimiter ``i_delimiter`` is not specified
     * then function simply returns ``i_limit`` objects order by name. Else
     * function recursively search objects order by name and all keys that contain
     * the same string between the ``i_prefix`` and the first occurrence of the
     * ``i_delimiter`` after the ``i_prefix`` are grouped under a single result element.
     *
     * NOTE: Recursive CTE (WITH RECURSIVE) is used instead of loop because it is faster.
     */
    v_prefix := coalesce(i_prefix, '');
    v_start_after := coalesce(i_start_after, '');
    v_escaped_prefix := regexp_replace(v_prefix, '([_%\\])', '\\\&', 'g');

    /*
     * If the specified start marker is a common prefix select the last key with
     * the common prefix as a start key.
     */
    IF i_delimiter IS NOT NULL THEN
        IF (v_start_after = v1_impl.get_key_common_prefix(v_start_after, v_prefix, i_delimiter))
        THEN
            /*
             * NOTE: Last key with the specified common prefix could not be found if
             * all objects with the common prefix were deleted between listing requests.
             *
             * Do nothing in this case, use ``v_start_after`` as is.
             */
            v_start_after := coalesce(v1_impl.get_last_prefix_key(i_bid, v_start_after),
                                      v_start_after);
        END IF;

        /*
         * NOTE: We use dynamic execution (EXECUTE operator) because in case of pg_pathman's
         * partitioned table it works faster than static plpgsql's SELECT operator which executes
         * via prepared statements. In case of SELECT operator pg_pathman doesn't know nothing
         * about query parameters and postgres forced to choose plan with all partitions (slower)
         * and pg_pathman's plan node RuntimeAppend controls partitions from which data will be
         * requested in runtime execution phase. In case of EXECUTE operator, query planning
         * performs on each query separately and pg_pathman knows parameters on planning phase
         * and he immediately choose one partition and makes simple plan faster and summary overhead
         * in many cases is less than overhead of RuntimeAppend node.
         */
        RETURN QUERY
            EXECUTE format($quote$
                WITH RECURSIVE objects AS (
                    SELECT * FROM (
                        SELECT
                        v1_impl.get_object_listing_item(
                            s3o,
                            %2$L, %3$L, /*v_prefix, i_delimiter*/
                            1
                        ) AS listing_item
                        FROM s3.objects s3o
                        WHERE bid = %1$L /*i_bid*/
                          AND name > %4$L /*v_start_after*/
                          AND name LIKE %6$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                          AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                        ORDER BY name ASC
                        LIMIT 1
                    ) a /*subquery because we need ORDER BY name ASC LIMIT 1 with UNION ALL*/
                    UNION ALL
                    SELECT
                        (SELECT
                            v1_impl.get_object_listing_item(
                                s3o,
                                %2$L, %3$L, /*v_prefix, i_delimiter*/
                                (objects.listing_item).level + 1
                            )
                            FROM s3.objects s3o
                            WHERE bid = %1$L /*i_bid*/
                                AND name > (objects.listing_item).start_after
                                AND name LIKE %6$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                                AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                            ORDER BY name LIMIT 1
                        ) AS listing_item
                    FROM objects
                    WHERE (objects.listing_item).object.name IS NOT NULL
                        AND (objects.listing_item).level < %5$L /*i_limit*/
                )
                SELECT (objects.listing_item).object.bid,
                        (objects.listing_item).object.name,
                        (objects.listing_item).object.created,
                        (objects.listing_item).object.cid,
                        (objects.listing_item).object.data_size,
                        (objects.listing_item).object.data_md5,
                        (objects.listing_item).object.mds_namespace,
                        (objects.listing_item).object.mds_couple_id,
                        (objects.listing_item).object.mds_key_version,
                        (objects.listing_item).object.mds_key_uuid,
                        (objects.listing_item).object.null_version,
                        (objects.listing_item).object.delete_marker,
                        (objects.listing_item).object.parts_count,
                        NULL::s3.object_part[] AS parts,
                        (objects.listing_item).object.storage_class,
                        (objects.listing_item).object.creator_id,
                        (objects.listing_item).object.metadata,
                        NULL::jsonb as acl,
                        NULL::jsonb as lock_settings
                    FROM objects
                    WHERE (objects.listing_item).level IS NOT NULL
                    ORDER BY (objects.listing_item).level
            $quote$, i_bid, v_prefix, i_delimiter, v_start_after,
                i_limit, v_escaped_prefix);
    ELSE
        -- Simply return ``i_limit`` objects order by name
        RETURN QUERY
            EXECUTE format($quote$
                SELECT bid, name, created, cid, data_size,
                    data_md5, mds_namespace, mds_couple_id, mds_key_version,
                    mds_key_uuid, null_version, delete_marker,
                    parts_count, NULL::s3.object_part[] AS parts, storage_class, creator_id,
                    metadata, NULL::jsonb as acl, NULL::jsonb as lock_settings
                FROM s3.objects
                WHERE bid = %1$L /*i_bid*/
                  AND name > %3$L /*v_start_after*/
                  AND name LIKE %5$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                  AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                ORDER BY name ASC
                LIMIT %4$L /*i_limit*/
            $quote$, i_bid, v_prefix, v_start_after,
                i_limit, v_escaped_prefix);
            -- NOTE: As we have unique index on (bid, name) there could be only
            -- one object with such name (no need to ORDER BY created DESC)
    END IF;

    RETURN;
END;
$function$;
