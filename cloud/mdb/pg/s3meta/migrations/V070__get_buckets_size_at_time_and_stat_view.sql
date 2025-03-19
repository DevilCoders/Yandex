/*
 * Returns bucket size (sum of recent s3.buckets_size and s3.buckets_usage) at i_target_ts, or
 *  min(last_counters_updated_ts) rounded to start of hour by default.
 */

CREATE OR REPLACE FUNCTION s3.get_buckets_size_at_time(
    i_target_ts timestamp with time zone DEFAULT NULL,
    i_raise_bad_target_ts bool DEFAULT FALSE
)
RETURNS TABLE (
    bid uuid,
    shard_id int,
    storage_class int,
    size bigint,
    target_ts timestamp with time zone
)
LANGUAGE plpgsql VOLATILE AS $function$
DECLARE
  v_target_ts timestamp with time zone;
BEGIN
  SELECT MIN(last_counters_updated_ts) FROM s3.parts INTO v_target_ts;
  v_target_ts = date_trunc('hour', v_target_ts);

  IF i_target_ts IS NOT NULL THEN
    IF i_raise_bad_target_ts THEN
      IF date_trunc('hour', i_target_ts) > v_target_ts THEN
        RAISE EXCEPTION 'i_target_ts must be less then %s', v_target_ts;
      END IF;
      IF date_trunc('hour', i_target_ts) != i_target_ts THEN
        RAISE EXCEPTION 'i_target_ts must be start of hour, for example %', date_trunc('hour', i_target_ts);
      END IF;
    END IF;
    v_target_ts = i_target_ts;
  END IF;

  RETURN QUERY
    WITH v AS (
      SELECT d.bid, d.shard_id, d.storage_class, d.base_ts, (d.base_size + sum(d.size_change))::bigint AS size
        FROM (
          SELECT
              coalesce(s.bid, u.bid) as bid,
              coalesce(s.shard_id, u.shard_id) as shard_id,
              coalesce(s.storage_class, u.storage_class) as storage_class,
              s.target_ts AS base_ts,
              coalesce(s.size, 0) AS base_size,
              coalesce(u.size_change, 0) as size_change
            FROM (
              -- if we haven't rows in buckets_size then bucket was created recently (after last update)
              SELECT DISTINCT i.bid, i.shard_id, i.storage_class, '1970-01-01 00:00:00'::timestamp AS target_ts, 0 as size
                FROM s3.buckets_usage i
              UNION
              -- get last buckets_size (max(target_ts) == target_ts)
              SELECT a.bid, a.shard_id, a.storage_class, a.target_ts, a.size
                FROM s3.buckets_size a
                JOIN (
                  SELECT bs.bid, bs.shard_id, bs.storage_class, max(bs.target_ts) AS target_ts
                    FROM s3.buckets_size bs
                    WHERE (bs.target_ts <= v_target_ts)
                    GROUP by bs.bid, bs.shard_id, bs.storage_class
                ) b
                USING (bid, shard_id, storage_class, target_ts)
              -- we got [bid, shard_id, storage_class, max(target_ts) or 1970, size at target_ts]
            ) s
            LEFT JOIN (
               SELECT bu.bid, bu.shard_id, bu.storage_class, bu.start_ts AS change_ts, bu.size_change
                 FROM s3.buckets_usage bu
                 WHERE end_ts <= v_target_ts
            ) u
            ON (u.bid=s.bid
                AND u.shard_id = s.shard_id
                AND u.storage_class = s.storage_class
                AND u.change_ts >= s.target_ts)
            -- we got [bid, shard_id, storage_class, some base_ts, size at base_ts, one of size change since base_ts]
        ) d
        GROUP BY d.bid, d.shard_id, d.storage_class, d.base_ts, d.base_size
        -- we got [bid, shard_id, storage_class, size at some base_ts, summary size change since base_ts]
    )
    -- we could get some rows for each bid with different base_ts => get recent (base_ts = max(base_ts))
    SELECT v.bid, v.shard_id, v.storage_class, v.size, v_target_ts
      FROM v JOIN (
        SELECT v2.bid, v2.shard_id, v2.storage_class, max(v2.base_ts) AS base_ts
          FROM v v2
          GROUP BY v2.bid, v2.shard_id, v2.storage_class
      ) m
      USING (bid, shard_id, storage_class, base_ts);
  RETURN;
END;
$function$;


DROP MATERIALIZED VIEW s3.bucket_storage_class_stat;
CREATE MATERIALIZED VIEW s3.bucket_storage_class_stat (
    bid,
    name,
    storage_class,
    size
) AS SELECT
      b.bid,
      b.name,
      storage_class,
      size
    FROM (
      SELECT bid, storage_class, sum(size)::bigint AS size
        FROM s3.get_buckets_size_at_time()
        GROUP BY bid, storage_class
    ) r JOIN s3.buckets b ON (r.bid = b.bid);

CREATE UNIQUE INDEX ON s3.bucket_storage_class_stat (name, storage_class);
