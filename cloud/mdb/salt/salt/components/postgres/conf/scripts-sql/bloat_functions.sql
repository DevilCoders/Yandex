CREATE OR REPLACE FUNCTION get_index_bloat_info()
RETURNS TABLE (
	dbname name, 
	schema_name name, 
	table_name name, 
	index_name name, 
	bloat_pct numeric,
	bloat_bytes numeric, 
	bloat_size text, 
	total_bytes numeric, 
	index_size text, 
	table_bytes bigint, 
	table_size text, 
	index_scans bigint
) AS $$ 
BEGIN

RETURN QUERY WITH btree_index_atts AS (
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
FROM raw_bloat r;
END;
$$ LANGUAGE PLPGSQL
    SECURITY DEFINER;

CREATE OR REPLACE FUNCTION get_heap_bloat_info()
RETURNS TABLE (
	current_database name, 
	schemaname name, 
	tblname name, 
	real_size text, 
	extra_size text, 
	extra_ratio float, 
	fillfactor integer, 
	bloat_size text, 
	bloat_size_bytes bigint, 
	bloat_ratio float, 
	is_na boolean 
) AS $$
BEGIN
RETURN QUERY WITH s AS (
SELECT
        tbl.oid AS tblid, 
	ns.nspname AS schemaname, 
	tbl.relname AS tblname, 
	tbl.reltuples,
        tbl.relpages AS heappages, 
	COALESCE(toast.relpages, 0) AS toastpages,
        COALESCE(toast.reltuples, 0) AS toasttuples,
        COALESCE(substring(array_to_string(tbl.reloptions, ' ')
          FROM '%fillfactor=#"__#"%' FOR '#')::smallint, 100) AS fillfactor,
        current_setting('block_size')::numeric AS bs,
        CASE WHEN version()~'mingw32' OR version()~'64-bit|x86_64|ppc64|ia64|amd64' 
	THEN 8 
	ELSE 4 
	END AS ma,
        24 AS page_hdr,
        23 + CASE WHEN MAX(coalesce(null_frac,0)) > 0 
	THEN ( 7 + count(*) ) / 8 
	ELSE 0::int 
	END
          + CASE WHEN tbl.relhasoids 
	THEN 4 
	ELSE 0 
	END AS tpl_hdr_size,
        sum( (1-coalesce(s.null_frac, 0)) * coalesce(s.avg_width, 1024) ) AS tpl_data_size,
        bool_or(att.atttypid = 'pg_catalog.name'::regtype) AS is_na
FROM pg_attribute AS att
        JOIN pg_class AS tbl ON att.attrelid = tbl.oid
        JOIN pg_namespace AS ns ON ns.oid = tbl.relnamespace
        JOIN pg_stats AS s ON s.schemaname=ns.nspname
          AND s.tablename = tbl.relname AND s.inherited=false AND s.attname=att.attname
        LEFT JOIN pg_class AS toast ON tbl.reltoastrelid = toast.oid
WHERE att.attnum > 0 AND NOT att.attisdropped
        AND tbl.relkind = 'r'
GROUP BY 
	1,2,3,4,5,6,7,8,9,10, tbl.relhasoids
ORDER BY 
	5 DESC
), s2 AS (
SELECT
      	( 4 + tpl_hdr_size + tpl_data_size + (2*ma)
        - CASE WHEN tpl_hdr_size%ma = 0 
	THEN ma 
	ELSE tpl_hdr_size%ma 
	END
        - CASE WHEN ceil(tpl_data_size)::int%ma = 0 
	THEN ma 
	ELSE ceil(tpl_data_size)::int%ma 
	END
      	) AS tpl_size, 
	bs - page_hdr AS size_per_block, 
	(heappages + toastpages) AS tblpages, 
	heappages,
      	toastpages, 
	reltuples, 
	toasttuples, 
	bs, 
	page_hdr, 
	tblid, 
	s.schemaname, 
	s.tblname, 
	s.fillfactor, 
	s.is_na
FROM s
), s3 AS (
SELECT 
	ceil( reltuples / ( (bs-page_hdr)/tpl_size ) ) + ceil( toasttuples / 4 ) AS est_tblpages,
    	ceil( reltuples / ( (bs-page_hdr)*s2.fillfactor/(tpl_size*100) ) ) + ceil( toasttuples / 4 ) AS est_tblpages_ff,
    	s2.tblpages,
    	s2.fillfactor,
    	s2.bs,
    	s2.tblid,
    	s2.schemaname,
    	s2.tblname,
    	s2.heappages,
    	s2.toastpages,
    	s2.is_na
FROM s2
) SELECT
	current_database(),
	s3.schemaname, 
	s3.tblname, 
	pg_size_pretty(bs*s3.tblpages) AS real_size,
	pg_size_pretty(((s3.tblpages-est_tblpages)*bs)::bigint) AS extra_size,
  	CASE WHEN tblpages - est_tblpages > 0
    	THEN 100 * (s3.tblpages - est_tblpages)/s3.tblpages::float
    	ELSE 0
  	END AS extra_ratio, 
	s3.fillfactor, 
	pg_size_pretty(((s3.tblpages-est_tblpages_ff)*bs)::bigint) AS bloat_size,
  	((tblpages-est_tblpages_ff)*bs)::bigint bytes_bloat_size,
  	CASE WHEN s3.tblpages - est_tblpages_ff > 0
    	THEN 100 * (s3.tblpages - est_tblpages_ff)/s3.tblpages::float
    	ELSE 0
  END AS bloat_ratio, s3.is_na
  FROM s3;

END;
$$ LANGUAGE PLPGSQL
    SECURITY DEFINER
