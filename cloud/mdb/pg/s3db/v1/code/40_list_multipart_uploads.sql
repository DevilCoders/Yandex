/*
 * Returns some or all (up to ``i_limit``) multipart uploads in a bucket's chunk.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *    Name and ID of the bucket to list objects from.
 * - i_prefix:
 *    Limits the response to in-progess multipart uploads only for those keys
 *    that begin with the specified prefix.
 * - i_delimiter:
 *    A single character (or group of characters) used to group keys.
 *    If ``i_prefix`` is specified, all keys that contain the same string between
 *    the ``i_prefix`` and the first occurrence of the ``i_delimiter`` after the
 *    ``i_prefix`` are grouped under a single result element. If ``i_prefix`` is
 *    not specified (i.e. NULL), the substring starts at the beginning of the key.
 *    All of the keys rolled up in a common prefix count as a single return when
 *    calculating the number of returns.
 *    Returned instance of ``code.object_part`` will have all attributes NULL-ed,
 *    but ``name`` that will contain a common prefix.
 * - i_start_after_key:
 *    Specifies the key (or a common prefix) to start listing after.
 * - i_start_after_created:
 *    Together with ``i_start_after_key`` specifies the multipart upload after
 *    which listing should begin. Any multipart upload for a key equal to the
 *    key specified by ``i_start_after_key`` might be included in the list only
 *    if they have ``object_created`` greater than ``i_start_after_created``.
 * - i_limit:
 *    Sets the maximum number of multipart uploads returned in the response.
 *
 * Returns:
 *   List of the bucket chunk's multipart uploads that satisfy the search criteria
 *   specified by ``i_prefix``, ``i_delimiter``, ``i_start_after_key``,
 *   ``i_start_after_created``. Each common prefix (or a "directory") will be
 *   represented as an instance of ``code.object_part`` with all attributes but
 *   name set to NULL.
 */
CREATE OR REPLACE FUNCTION v1_code.list_multipart_uploads(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text DEFAULT NULL,
    i_delimiter text DEFAULT NULL,
    i_start_after_key text DEFAULT NULL,
    i_start_after_created timestamp with time zone DEFAULT NULL,
    i_limit integer DEFAULT 1000
) RETURNS SETOF v1_code.multipart_upload
LANGUAGE plpgsql STABLE AS $function$
DECLARE
    v_prefix text;
    v_start_after_key text;
    v_start_after_created timestamp with time zone;
    v_delimiter_position integer;
    v_current_upload v1_code.multipart_upload;
    v_escaped_prefix text;
BEGIN
    /*
     * The function works as follows. On each iteration the next result element
     * is searched for based on the last found result element (``v_start_after_key``,
     * ``v_start_after_created``). The next result is searched for according to
     * ``i_prefix`` and ``i_delimiter`` restrictions.
     * Once the next result element is found (either an actual key or a common
     * prefix) it updates ``v_start_after_key`` and ``v_start_after_created``
     * with the found upload and decrements ``v_limit``.
     *
     * NOTE: Loop is used instead of a window function because window function is
     * applied to the whole selection after processing of other conditions (like
     * WHERE and ORDER BY).
     */
    v_prefix := coalesce(i_prefix, '');
    v_start_after_key := coalesce(i_start_after_key, '');
    v_start_after_created := i_start_after_created;
    v_escaped_prefix := regexp_replace(v_prefix, '([_%\\])', '\\\&', 'g');

    /*
     * If the specified start marker is a common prefix select the last key with
     * the common prefix as a start key.
     */
    IF i_delimiter IS NOT NULL THEN
        IF v_start_after_key = v1_impl.get_key_common_prefix(v_start_after_key, v_prefix, i_delimiter) THEN
            /*
             * NOTE: Last key with the specified common prefix could not be found if
             * all multipart uploads with the common prefix were completed/aborted
             * between listing requests.
             *
             * Do nothing in this case, use ``v_start_after_key`` as is.
             */
            v_start_after_key := coalesce(v1_impl.get_last_prefix_upload(i_bid, v_start_after_key),
                                          v_start_after_key);
            v_start_after_created := NULL;
        END IF;

        /*
         * NOTE: We use dynamic execution (EXECUTE operator) because in case of pg_pathman's
         * partititoned table it works faster than static plpgsql's SELECT operator which executes
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
                WITH RECURSIVE object_parts AS (
                    SELECT * FROM (
                        SELECT
                        v1_impl.get_object_part_listing_item(
                            s3op,
                            %2$L, %3$L, /*v_prefix, i_delimiter*/
                            1
                        ) AS listing_item
                        FROM s3.object_parts s3op
                        WHERE bid = %1$L /*i_bid*/
                            AND part_id = 0
                            AND (name > %4$L /*v_start_after_key*/
                                OR (%5$L IS NOT NULL /*v_start_after_created*/
                                    AND name = %4$L /*v_start_after_key*/
                                    AND object_created > %5$L) /*v_start_after_created*/
                            )
                            AND name LIKE %7$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                            AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                        ORDER BY name ASC
                        LIMIT 1
                    ) a /*subquery because we need ORDER BY name ASC LIMIT 1 with UNION ALL*/
                    UNION ALL
                    SELECT
                        (SELECT
                            v1_impl.get_object_part_listing_item(
                                s3op,
                                %2$L, %3$L, /*v_prefix, i_delimiter*/
                                (object_parts.listing_item).level + 1
                            )
                            FROM s3.object_parts s3op
                            WHERE bid = %1$L /*i_bid*/
                                AND part_id = 0
                                AND (name > (object_parts.listing_item).start_after_key
                                    OR ((object_parts.listing_item).start_after_created IS NOT NULL
                                        AND name = (object_parts.listing_item).start_after_key
                                        AND object_created > (object_parts.listing_item).start_after_created)
                                )
                                AND name LIKE %7$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                                AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                            ORDER BY s3op.name LIMIT 1
                        ) AS listing_item
                    FROM object_parts
                    WHERE (object_parts.listing_item).object_part.name IS NOT NULL
                        AND (object_parts.listing_item).level < %6$L /*i_limit*/
                )
                SELECT (object_parts.listing_item).object_part.bid,
                        (object_parts.listing_item).object_part.cid,
                        (object_parts.listing_item).object_part.name,
                        (object_parts.listing_item).object_part.object_created,
                        (object_parts.listing_item).object_part.part_id,
                        (object_parts.listing_item).object_part.created,
                        NULL::bigint AS data_size,
                        (object_parts.listing_item).object_part.data_md5,
                        (object_parts.listing_item).object_part.mds_namespace,
                        (object_parts.listing_item).object_part.mds_couple_id,
                        (object_parts.listing_item).object_part.mds_key_version,
                        (object_parts.listing_item).object_part.mds_key_uuid,
                        (object_parts.listing_item).object_part.storage_class,
                        (object_parts.listing_item).object_part.creator_id,
                        NULL::jsonb as metadata, NULL::jsonb as acl, NULL::jsonb as lock_settings
                    FROM object_parts
                    WHERE (object_parts.listing_item).level IS NOT NULL
                    ORDER BY (object_parts.listing_item).level
            $quote$, i_bid, v_prefix, i_delimiter, v_start_after_key,
                v_start_after_created, i_limit, v_escaped_prefix);
    ELSE
        RETURN QUERY
            EXECUTE format($quote$
                SELECT bid, cid, name, object_created, part_id, created, NULL::bigint as data_size, data_md5,
                        mds_namespace, mds_couple_id, mds_key_version, mds_key_uuid, storage_class, creator_id,
                        NULL::jsonb as metadata, NULL::jsonb as acl, NULL::jsonb as lock_settings
                    FROM s3.object_parts
                    WHERE bid = %1$L /*i_bid*/
                      AND part_id = 0
                      AND (name > %3$L /*v_start_after_key*/
                           OR (%4$L IS NOT NULL /*v_start_after_created*/
                               AND name = %3$L /*v_start_after_key*/
                               AND object_created > %4$L)) /*v_start_after_created*/
                      AND name LIKE %6$L || '%%' ESCAPE '\' /*v_escaped_prefix*/
                      AND left(name, char_length(%2$L)) = %2$L /*v_prefix*/
                    ORDER BY name ASC, object_created ASC
                    LIMIT %5$L /*i_limit*/
            $quote$, i_bid, v_prefix, v_start_after_key,
                v_start_after_created, i_limit, v_escaped_prefix);
    END IF;

    RETURN;
END;
$function$;
