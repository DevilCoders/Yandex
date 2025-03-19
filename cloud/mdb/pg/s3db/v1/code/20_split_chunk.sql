CREATE OR REPLACE FUNCTION v1_code.split_chunk(
    i_bid uuid,
    i_cid bigint,
    i_new_cid bigint,
    i_max_objects bigint DEFAULT 50000,
    i_split_key text DEFAULT NULL
) RETURNS v1_code.chunk
LANGUAGE plpgsql VOLATILE AS $function$
DECLARE
    v_start_key text;
    v_end_key text;
    v_objects_count bigint;
    v_chunk v1_code.chunk;
    v_limit bigint;
    v_split_key text;
    v_chunks_counters_changes v1_code.chunk_counters[];
    v_curr_counters v1_code.chunk_counters;
BEGIN

    SELECT start_key, end_key
        INTO v_start_key, v_end_key
        FROM s3.chunks WHERE bid = i_bid AND cid = i_cid;

    SELECT sum(simple_objects_count + multipart_objects_count)
        INTO v_objects_count
        FROM s3.chunks_counters WHERE bid = i_bid AND cid = i_cid;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such chunk'
            USING ERRCODE = 'S3X01';
    END IF;

    IF v_objects_count <= 0 THEN
        RAISE EXCEPTION 'Could not split chunk with % objects',
            v_objects_count
            USING ERRCODE = 'S3X02';
    END IF;

    v_limit := least(v_objects_count / 2, i_max_objects);

    IF i_split_key IS NULL THEN
        /* Calculate split_key if not specified */
        EXECUTE format($quote$
            SELECT min(name) AS key FROM (
                SELECT
                    name
                FROM (
                    SELECT name
                    FROM (
                        SELECT name
                            FROM s3.objects
                            WHERE bid = %1$L /* i_bid */
                                AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
                                AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
                        ORDER BY name DESC
                        LIMIT %4$s
                    ) x
                    UNION ALL
                    SELECT name
                    FROM (
                        SELECT name
                            FROM s3.objects_noncurrent
                            WHERE bid = %1$L /* i_bid */
                                AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
                                AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
                        ORDER BY name DESC
                        LIMIT %4$s
                    ) x
                    UNION ALL
                    SELECT name
                    FROM (
                        SELECT name
                            FROM s3.object_delete_markers
                            WHERE bid = %1$L /* i_bid */
                                AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
                                AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
                        ORDER BY name DESC
                        LIMIT %4$s
                    ) x
                ) a
                ORDER BY name DESC
                LIMIT %4$s
            ) b WHERE (%2$L IS NULL OR name > %2$L)
        $quote$, i_bid, v_start_key, v_end_key, v_limit)
        INTO v_split_key;
    ELSE
        v_split_key := i_split_key;
    END IF;

    IF v_split_key IS NULL THEN
        RAISE EXCEPTION 'Cannot split this chunk'
        USING ERRCODE = 23503;
    END IF;

    /* Select target ranges of data in object tables for warming up before blocking the chunk */
    EXECUTE format($quote$
        SELECT
            sum(data_size)
        FROM s3.objects
        WHERE bid = %1$L /* i_bid */
            AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
            AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
            AND (name >= %4$L) /* v_split_key */
        UNION ALL
        SELECT
            sum(data_size)
        FROM s3.objects_noncurrent
        WHERE bid = %1$L /* i_bid */
            AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
            AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
            AND (name >= %4$L) /* v_split_key */
        UNION ALL
        SELECT
            sum(length(name))
        FROM s3.object_delete_markers
        WHERE bid = %1$L /* i_bid */
            AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
            AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
            AND (name >= %4$L) /* v_split_key */
    $quote$, i_bid, v_start_key, v_end_key, v_split_key);

    /*
     * Block chunk and chunks_counters FOR UPDATE to prevent any operations
     * with objects because we need to calculate counters
     * correctly for new chunk
     */
    SELECT start_key, end_key
        INTO v_start_key, v_end_key
        FROM s3.chunks WHERE bid = i_bid AND cid = i_cid
    FOR UPDATE;

    PERFORM
        FROM s3.chunks_counters WHERE bid = i_bid AND cid = i_cid
    FOR UPDATE;

    /* Calculate counters changes */
    EXECUTE format($quote$
        WITH agg_objects AS (
            SELECT
                count(1) FILTER (WHERE NOT is_multipart) AS simple_objects_count,
                coalesce(sum(data_size) FILTER (WHERE NOT is_multipart), 0) AS simple_objects_size,
                count(1) FILTER (WHERE is_multipart) AS multipart_objects_count,
                coalesce(sum(data_size) FILTER (WHERE is_multipart), 0) AS multipart_objects_size,
                coalesce(storage_class, 0) AS storage_class
            FROM (
                SELECT
                    name,
                    data_size,
                    is_multipart,
                    storage_class
                FROM (
                    SELECT
                        name,
                        data_size,
                        coalesce(parts_count, 0) > 0 AS is_multipart,
                        storage_class
                        FROM s3.objects
                        WHERE bid = %1$L /* i_bid */
                            AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
                            AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
                            AND (name >= %4$L) /* v_split_key */
                    UNION ALL
                    SELECT name,
                        CASE WHEN delete_marker THEN length(name) ELSE data_size END AS data_size,
                        coalesce(parts_count, 0) > 0 AS is_multipart,
                        storage_class
                        FROM s3.objects_noncurrent
                        WHERE bid = %1$L /* i_bid */
                            AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
                            AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
                            AND (name >= %4$L) /* v_split_key */
                    UNION ALL
                    SELECT name, length(name) AS data_size, false AS is_multipart, 0
                        FROM s3.object_delete_markers
                        WHERE bid = %1$L /* i_bid */
                            AND (%2$L IS NULL OR name >= %2$L) /* v_start_key */
                            AND (%3$L IS NULL OR name < %3$L)  /* v_end_key */
                            AND (name >= %4$L) /* v_split_key */
                ) a
            ) b
            GROUP BY coalesce(storage_class, 0)
        ),
        agg_parts AS (
            SELECT
                count(1) FILTER (WHERE part_id > 0) AS objects_parts_count,
                coalesce(sum(data_size) FILTER (WHERE part_id > 0), 0)::bigint AS objects_parts_size,
                count(1) FILTER (WHERE part_id = 0) AS active_multipart_count,
                coalesce(storage_class, 0) AS storage_class
            FROM s3.object_parts
            WHERE bid = %1$L
                AND name >= %4$L
                AND (%3$L IS NULL OR name < %3$L)
            GROUP BY coalesce(storage_class, 0)
        ),
        arr AS (
            SELECT (%1$L, NULL /* cid */,
                coalesce(agg_objects.simple_objects_count, 0),
                coalesce(agg_objects.simple_objects_size, 0),
                coalesce(agg_objects.multipart_objects_count, 0),
                coalesce(agg_objects.multipart_objects_size, 0),
                coalesce(agg_parts.objects_parts_count, 0),
                coalesce(agg_parts.objects_parts_size, 0),
                0, 0,
                coalesce(agg_parts.active_multipart_count, 0),
                storage_class
            )::v1_code.chunk_counters AS row
            FROM agg_objects FULL OUTER JOIN agg_parts USING (storage_class)
        )
        SELECT ARRAY(SELECT * FROM arr)
    $quote$, i_bid, v_start_key, v_end_key, v_split_key)
    INTO v_chunks_counters_changes;

    /* Create new chunk */
    SELECT * INTO v_chunk
        FROM v1_code.add_chunk(
            i_bid, i_new_cid,
            v_split_key, /* start_key */
            v_end_key /* end_key */
        );

    /* Update chunk keys */
    UPDATE s3.chunks SET end_key = v_split_key
        WHERE bid = i_bid AND cid = i_cid;

    /* Update chunks counters */
    FOREACH v_curr_counters IN ARRAY v_chunks_counters_changes
    LOOP
        v_curr_counters.cid = i_cid;
        PERFORM v1_code.chunks_counters_queue_push(OPERATOR(v1_code.-) v_curr_counters);
        v_curr_counters.cid = i_new_cid;
        PERFORM v1_code.chunks_counters_queue_push(v_curr_counters);
    END LOOP;

    RETURN v_chunk;
END;
$function$;
