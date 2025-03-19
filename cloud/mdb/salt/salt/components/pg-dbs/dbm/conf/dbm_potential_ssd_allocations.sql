WITH hosts AS (
  SELECT
    i.generation AS gen,
    i.geo,
    i.free_ssd   AS free_ssd
  FROM mdb.dom0_info i
  WHERE (i.project = 'pgaas')
    AND heartbeat > now()::DATE - 1
),

     arr AS (SELECT ARRAY [
                      CAST(10 * power(2, 30) AS BIGINT),
                      CAST(16 * power(2, 30) AS BIGINT),
                      CAST(32 * power(2, 30) AS BIGINT),
                      CAST(64 * power(2, 30) AS BIGINT),
                      CAST(128 * power(2, 30) AS BIGINT),
                      CAST(256 * power(2, 30) AS BIGINT),
                      CAST(512 * power(2, 30) AS BIGINT),
                      CAST(768 * power(2, 30) AS BIGINT),
                      CAST(1024 * power(2, 30) AS BIGINT),
                      CAST(1536 * power(2, 30) AS BIGINT),
                      CAST(2048 * power(2, 30) AS BIGINT),
                      CAST(4096 * power(2, 30) AS BIGINT)
                      ] AS arr),

     resources as
       (
         SELECT unnest(a.arr) AS u,
                h.*
         FROM hosts AS h
                CROSS JOIN (SELECT arr FROM arr) AS a
       ),
     palloc AS (
       SELECT r.gen,
              r.geo,
              u / power(2, 30) bucket,
              floor(free_ssd / u) AS alloc_count
       FROM resources AS r
     )
SELECT gen, geo, bucket AS ssd_size, sum(alloc_count) AS potential_allocations
FROM palloc
GROUP BY gen, geo, bucket
