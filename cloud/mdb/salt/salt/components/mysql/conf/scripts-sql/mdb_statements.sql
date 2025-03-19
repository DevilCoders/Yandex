use sys;
create or replace DEFINER=`mysql.sys`@`localhost` SQL SECURITY INVOKER view sys.mdb_statements as
SELECT  COALESCE(schema_name, '-')            AS db,
            digest                                AS digest,
            digest_text                           AS query,
            count_star                            AS calls,
            ROUND(sum_timer_wait / 1000000000, 2) AS total_query_latency,
            ROUND(sum_lock_time / 1000000000, 2)  AS total_lock_latency,
            sum_errors                            AS errors,
            sum_warnings                          AS warnings,
            sum_rows_affected                     AS rows_affected,
            sum_rows_sent                         AS rows_sent,
            sum_rows_examined                     AS rows_examined,
            sum_created_tmp_disk_tables           AS tmp_disk_tables,
            sum_created_tmp_tables                AS tmp_tables,
            sum_select_full_join                  AS select_full_join,
            sum_select_full_range_join            AS select_full_range_join,
            sum_select_range                      AS select_range,
            sum_select_range_check                AS select_range_check,
            sum_select_scan                       AS select_scan,
            sum_sort_merge_passes                 AS sort_merge_passes,
            sum_sort_range                        AS sort_range,
            sum_sort_rows                         AS sort_rows,
            sum_sort_scan                         AS sort_scan,
            sum_no_index_used                     AS no_index_used,
            sum_no_good_index_used                AS no_good_index_used
    FROM performance_schema.events_statements_summary_by_digest
    WHERE schema_name NOT IN ('mysql', 'information_schema', 'sys', 'performance_schema');
