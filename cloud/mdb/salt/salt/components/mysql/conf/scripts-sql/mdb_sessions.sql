use sys;
create or replace DEFINER=`mysql.sys`@`localhost` SQL SECURITY INVOKER view sys.mdb_sessions as
SELECT thd.thread_id                                                                      AS thd_id,
           thd.processlist_id                                                                 AS conn_id,
           thd.processlist_user                                                               AS user,
           COALESCE(thd.processlist_db, '-')                                                  AS db,
           thd.processlist_command                                                            AS command,
           COALESCE(SUBSTR(plist.INFO, 1, 65000), thd.processlist_info)                       AS query,
           stm.digest                                                                         AS digest,
           ROUND(stm.timer_wait / 1000000000, 2)                                              AS query_latency,
           ROUND(stm.lock_time / 1000000000, 2)                                               AS lock_latency,
           IF(esc.end_event_id IS NULL, esc.event_name, NULL)                                 AS stage,
           IF(esc.end_event_id IS NULL, ROUND(esc.timer_wait / 1000000000, 2), NULL)          AS stage_latency,
           IF(ewc.end_event_id IS NULL AND ewc.event_name IS NOT NULL AND ewc.event_name != 'idle', ewc.event_name, NULL)  AS current_wait,
           IF(ewc.end_event_id IS NULL AND ewc.event_name IS NOT NULL AND ewc.event_name != 'idle',
              CASE
                  WHEN ewc.object_name LIKE '/var/lib/mysql/mysql-bin-log%' THEN '/var/lib/mysql/mysql-bin-log-XXXXX'
                  WHEN ewc.object_name LIKE '/var/lib/mysql/mysql-relay-log%' THEN '/var/lib/mysql/mysql-relay-log-XXXXX'
                  WHEN ewc.object_name LIKE '/var/lib/mysql/.tmp/%' THEN '/var/lib/mysql/.tmp/XXXXX'
                  WHEN ewc.event_name = 'wait/io/socket/sql/client_connection' THEN NULL
                  ELSE ewc.object_name
              END,
              NULL)                                                                           AS wait_object,
           IF(ewc.end_event_id IS NULL AND ewc.event_name IS NOT NULL AND ewc.event_name != 'idle',
              ROUND(ewc.timer_wait / 1000000000, 2),
              NULL)                                                                           AS wait_latency,
           IF(etc.state = 'ACTIVE', ROUND(etc.timer_wait / 1000000000, 2), NULL)              AS trx_latency,
           mem.current_memory                                                                 AS current_memory,
           skt.addr                                                                           AS client_addr,
           COALESCE(hc.host, skt.addr)                                                        AS client_hostname,
           skt.port                                                                           AS client_port
    FROM performance_schema.threads thd
    LEFT JOIN information_schema.processlist plist ON thd.processlist_id = plist.id
    LEFT JOIN performance_schema.events_stages_current esc ON thd.thread_id = esc.thread_id
    LEFT JOIN performance_schema.events_transactions_current etc ON thd.thread_id = etc.thread_id
    LEFT JOIN performance_schema.events_statements_current stm ON thd.thread_id = stm.thread_id
    LEFT JOIN (
        SELECT thread_id, SUM(current_number_of_bytes_used) AS current_memory
        FROM performance_schema.memory_summary_by_thread_by_event_name
        GROUP BY thread_id
        HAVING current_memory > 0
      ) mem ON thd.thread_id = mem.thread_id
    LEFT JOIN (
        SELECT *
        FROM performance_schema.events_waits_current
        WHERE event_id = (
          SELECT MAX(event_id)
          FROM performance_schema.events_waits_current w2
          WHERE w2.thread_id = events_waits_current.thread_id
        )
    ) ewc ON thd.thread_id = ewc.thread_id
    LEFT JOIN (
        SELECT thread_id, port, IF(ip LIKE '::ffff:%', substr(ip, 8, length(ip)), ip) AS addr
        FROM performance_schema.socket_instances
    ) skt ON skt.thread_id = thd.thread_id
    LEFT JOIN performance_schema.host_cache hc ON hc.ip = skt.addr
    WHERE thd.name = 'thread/sql/one_connection'
      AND thd.processlist_command IS NOT NULL AND thd.processlist_command NOT IN ('Sleep', 'Binlog Dump GTID', 'Binlog Dump')
      AND thd.processlist_user NOT IN ('repl', 'admin', 'monitor');
