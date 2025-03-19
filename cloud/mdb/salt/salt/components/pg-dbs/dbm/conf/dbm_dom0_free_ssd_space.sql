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
                      CAST(0 * power(2, 30) AS BIGINT),
                      CAST(64 * power(2, 30) AS BIGINT),
                      CAST(256 * power(2, 30) AS BIGINT),
                      CAST(512 * power(2, 30) AS BIGINT),
                      CAST(1 * power(2, 40) AS BIGINT),
                      CAST(2 * power(2, 40) AS BIGINT),
                      CAST(4 * power(2, 40) AS BIGINT),
                      CAST(6 * power(2, 40) AS BIGINT),
                      CAST(8 * power(2, 40) AS BIGINT),
                      CAST(10 * power(2, 40) AS BIGINT)
                      ] AS arr),


     histogram AS
       (
         SELECT gen,
                geo,
                width_bucket(free_ssd, (SELECT arr FROM arr)) AS bucket,
                COUNT(*)                                      AS count
         FROM hosts
         GROUP BY geo, gen, bucket
       ) SELECT
     geo,
     gen,
     (SELECT arr FROM arr) [ bucket] / power(2, 30) AS ssd_space,
     count AS dom0_free_ssd_space_count
  FROM
     histogram;
