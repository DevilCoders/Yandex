WITH btree_index_atts AS (
    SELECT
    nspname,
    relname,
    reltuples,
    relpages,
    indrelid,
    relam,
        regexp_split_to_table(indkey::text, ' ')::smallint AS attnum,
        indexrelid as index_oid
    FROM pg_index
    JOIN pg_class ON pg_class.oid=pg_index.indexrelid
    JOIN pg_namespace ON pg_namespace.oid = pg_class.relnamespace
    JOIN pg_am ON pg_class.relam = pg_am.oid
    WHERE pg_am.amname = 'btree'
    ),
index_item_sizes AS (
    SELECT
        i.nspname,
        i.relname,
    i.reltuples,
    i.relpages,
    i.relam,
        s.starelid,
    a.attrelid AS table_oid,
    index_oid,
        current_setting('block_size')::numeric AS bs,
        /* MAXALIGN: 4 on 32bits, 8 on 64bits (and mingw32 ?) */
        CASE
        WHEN version() ~ 'mingw32' OR version() ~ '64-bit'
    THEN 8
        ELSE 4
        END AS maxalign,
        24 AS pagehdr,
        /* per tuple header: add index_attribute_bm if some cols are null-able */
        CASE WHEN max(coalesce(s.stanullfrac,0)) = 0
    THEN 2
    ELSE 6
        END AS index_tuple_hdr,
        /* data len: we remove null values save space using it fractionnal part from stats */
        SUM( (1-coalesce(s.stanullfrac, 0)) * coalesce(s.stawidth, 2048) ) AS nulldatawidth
        FROM pg_attribute AS a
        JOIN pg_statistic AS s ON s.starelid=a.attrelid AND s.staattnum = a.attnum
        JOIN btree_index_atts AS i ON i.indrelid = a.attrelid AND a.attnum = i.attnum
        WHERE
        a.attnum > 0
        GROUP BY
        1, 2, 3, 4, 5, 6, 7, 8, 9
),
index_aligned AS (
    SELECT
    maxalign,
    bs,
    nspname,
    relname AS index_name,
    reltuples,
        relpages,
    relam,
    table_oid,
    index_oid,
          ( 2 + maxalign -
    CASE /* Add padding to the index tuple header to align on MAXALIGN */
            WHEN index_tuple_hdr%maxalign = 0
        THEN maxalign
            ELSE index_tuple_hdr%maxalign
          END
        + nulldatawidth + maxalign -
    CASE /* Add padding to the data to align on MAXALIGN */
            WHEN nulldatawidth::integer%maxalign = 0 THEN maxalign
            ELSE nulldatawidth::integer%maxalign
          END
          )::numeric AS nulldatahdrwidth,
    pagehdr
    FROM index_item_sizes AS s1
),
otta_calc AS (
  SELECT
    s2.bs,
    s2.nspname,
    s2.table_oid,
    s2.index_oid,
    s2.index_name,
    s2.relpages,
    COALESCE(CEIL((reltuples*(4+nulldatahdrwidth))/(bs-pagehdr::float)) +
          CASE WHEN am.amname IN ('hash','btree')
    THEN 1
    ELSE 0
    END ,
    0 -- btree and hash have a metadata reserved block
    ) AS otta
  FROM index_aligned AS s2
    LEFT JOIN pg_am am ON s2.relam = am.oid
),
raw_bloat AS (
    SELECT
    current_database() AS dbname,
    nspname,
    c.relname AS table_name,
    sub.index_name,
        bs*(sub.relpages)::bigint AS totalbytes,
        CASE
            WHEN sub.relpages <= otta THEN 0
            ELSE bs*(sub.relpages-otta)::bigint END
            AS wastedbytes,
        CASE
            WHEN sub.relpages <= otta
            THEN 0
        ELSE bs*(sub.relpages-otta)::bigint * 100 / (bs*(sub.relpages)::bigint)
        END
        AS realbloat,
        pg_relation_size(sub.table_oid) AS table_bytes,
        stat.idx_scan AS index_scans
    FROM otta_calc AS sub
    JOIN pg_class AS c ON c.oid=sub.table_oid
    JOIN pg_stat_user_indexes AS stat ON sub.index_oid = stat.indexrelid
)
SELECT
    r.dbname AS database_name,
    nspname AS schema_name,
    r.table_name,
    r.index_name,
        round(realbloat, 1) AS bloat_pct,
        wastedbytes AS bloat_bytes,
    pg_size_pretty(wastedbytes::bigint) AS bloat_size,
        totalbytes AS index_bytes,
    pg_size_pretty(totalbytes::bigint) AS index_size,
        r.table_bytes,
    pg_size_pretty(r.table_bytes) AS table_size,
        r.index_scans
FROM raw_bloat r ORDER BY bloat_bytes DESC;
