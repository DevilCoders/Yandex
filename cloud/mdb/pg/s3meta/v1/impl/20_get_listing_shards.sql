/*
 * Returns all shards which can contain objects for specified listing parameters.
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Specify bucket name and id.
 * - i_prefix:
 *     A prefix that an appropriate key should start with.
 * - i_start_after:
 *     A key that an appropriate key should be greater than.
 *
 * Returns:
 *   List of shards in order to make requests.
 */
CREATE OR REPLACE FUNCTION v1_impl.get_listing_shards(
    i_bucket_name text,
    i_bid uuid,
    i_prefix text DEFAULT NULL,
    i_start_after text DEFAULT NULL
) RETURNS SETOF integer STABLE
LANGUAGE plpgsql AS $function$
BEGIN
    /*
     * We can use one query for all cases like this:
     *   RETURN QUERY EXECUTE format($$
     *       SELECT shard_id FROM (
     *           SELECT v1_code.get_object_shard(
     *               %1$L / i_bucket_name /,
     *               coalesce(%3$L / i_prefix /, %4$L / i_start_after /, '')
     *           ) AS shard_id, NULL::text AS start_key
     *           UNION ALL
     *           SELECT shard_id, start_key FROM s3.chunks
     *               WHERE bid = %2$L / i_bid /
     *                   AND (
     *                       %3$L IS NULL OR (
     *                           start_key <= %3$L
     *                           AND end_key >= %3$L
     *                           AND left(start_key, char_length(%3$L)) <= %3$L
     *                           AND left(end_key, char_length(%3$L)) >= %3$L
     *                       )
     *                   ) / i_prefix /
     *                   AND (%4$L IS NULL OR end_key >= %4$L) / i_start_after /
     *           ORDER BY start_key NULLS FIRST
     *       ) shards
     *       GROUP BY shard_id
     *       ORDER BY min(start_key)
     *   $$, i_bucket_name, i_bid, i_prefix, i_start_after);
     *
     * Due to complicated WHERE conditions with OR it can lead to bad generic plan
     * and we need to use EXECUTE to prevent PREPARE in PL/pgSQL.
     * But it is more complicated than 4 simple queries for each case.
     */

    IF i_prefix IS NULL AND i_start_after IS NULL THEN
        /*
         * Simple case: we need to return all shards in key order.
         */
         RETURN QUERY EXECUTE format($quote$
            WITH RECURSIVE shards AS (
                SELECT shard_id, min_key
                    FROM v1_impl.get_next_listing_shard(%1$L /* i_bid */, -1 /* non-existent shard_id */)
                UNION ALL
                SELECT (v1_impl.get_next_listing_shard(%1$L /* i_bid */, shards.shard_id)).*
                    FROM shards
                    WHERE shards.shard_id IS NOT NULL
            )
            SELECT shard_id FROM shards
                WHERE shard_id IS NOT NULL
            ORDER BY min_key
        $quote$, i_bid);

    ELSIF i_prefix IS NULL AND i_start_after IS NOT NULL THEN
        /*
         * Start_after specified without prefix. We need to filter chunks by end_key.
         */
         RETURN QUERY
            SELECT shard_id FROM (
                /*
                 * Use get_object_shard to include chunks (NULL, x), [x, NULL)
                 * which will be filtered by end_key > i_start_after condition.
                 */
                SELECT
                    v1_code.get_object_shard(i_bucket_name, i_start_after) AS shard_id,
                    -1 AS num
                UNION ALL
                SELECT
                    shard_id,
                    row_number() OVER (ORDER BY end_key ASC NULLS LAST) AS num
                FROM s3.chunks
                    WHERE bid = i_bid
                    AND (end_key > i_start_after OR end_key IS NULL)
            ) shards
            GROUP BY shard_id
            ORDER BY min(num);

    ELSIF i_prefix IS NOT NULL AND i_start_after IS NULL THEN
        /*
         * Prefix specified without start_after. We need to filter chunks by both keys.
         */
        RETURN QUERY
            SELECT shard_id FROM (
                SELECT
                    v1_code.get_object_shard(i_bucket_name, i_prefix) AS shard_id,
                    -1 AS num
                UNION ALL
                SELECT
                    shard_id,
                    row_number() OVER (ORDER BY coalesce(start_key, '') ASC) AS num
                FROM s3.chunks
                    WHERE bid = i_bid
                        AND (end_key >= i_prefix OR end_key IS NULL)
                        AND left(coalesce(start_key, ''), char_length(i_prefix)) <= i_prefix
            ) shards
            GROUP BY shard_id
            ORDER BY min(num);

    ELSE
        /*
         * Prefix and start_after specified. We need to filter chunks by both keys.
         */
        RETURN QUERY
            SELECT shard_id FROM (
                SELECT
                    v1_code.get_object_shard(i_bucket_name, i_start_after) AS shard_id,
                    -1 AS num
                UNION ALL
                SELECT
                    shard_id,
                    row_number() OVER (ORDER BY coalesce(start_key, '') ASC) AS num
                FROM s3.chunks
                    WHERE bid = i_bid
                        AND (end_key >= i_prefix OR end_key IS NULL)
                        AND left(coalesce(start_key, ''), char_length(i_prefix)) <= i_prefix
                        AND (end_key > i_start_after OR end_key IS NULL)
            ) shards
            GROUP BY shard_id
            ORDER BY min(num);
    END IF;
    RETURN;
END;
$function$;
