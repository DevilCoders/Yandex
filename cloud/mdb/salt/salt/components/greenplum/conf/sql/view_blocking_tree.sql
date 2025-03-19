CREATE OR REPLACE VIEW blocking_tree AS
WITH RECURSIVE
  lock_composite(requested, current) AS (VALUES
     ('AccessShareLock'::text, 'AccessExclusiveLock'::text),
     ('RowShareLock'::text, 'ExclusiveLock'::text),
     ('RowShareLock'::text, 'AccessExclusiveLock'::text),
     ('RowExclusiveLock'::text, 'ShareLock'::text),
     ('RowExclusiveLock'::text, 'ShareRowExclusiveLock'::text),
     ('RowExclusiveLock'::text, 'ExclusiveLock'::text),
     ('RowExclusiveLock'::text, 'AccessExclusiveLock'::text),
     ('ShareUpdateExclusiveLock'::text, 'ShareUpdateExclusiveLock'::text),
     ('ShareUpdateExclusiveLock'::text, 'ShareLock'::text),
     ('ShareUpdateExclusiveLock'::text, 'ShareRowExclusiveLock'::text),
     ('ShareUpdateExclusiveLock'::text, 'ExclusiveLock'::text),
     ('ShareUpdateExclusiveLock'::text, 'AccessExclusiveLock'::text),
     ('ShareLock'::text, 'RowExclusiveLock'::text),
     ('ShareLock'::text, 'ShareUpdateExclusiveLock'::text),
     ('ShareLock'::text, 'ShareRowExclusiveLock'::text),
     ('ShareLock'::text, 'ExclusiveLock'::text),
     ('ShareLock'::text, 'AccessExclusiveLock'::text),
     ('ShareRowExclusiveLock'::text, 'RowExclusiveLock'::text),
     ('ShareRowExclusiveLock'::text, 'ShareUpdateExclusiveLock'::text),
     ('ShareRowExclusiveLock'::text, 'ShareLock'::text),
     ('ShareRowExclusiveLock'::text, 'ShareRowExclusiveLock'::text),
     ('ShareRowExclusiveLock'::text, 'ExclusiveLock'::text),
     ('ShareRowExclusiveLock'::text, 'AccessExclusiveLock'::text),
     ('ExclusiveLock'::text, 'RowShareLock'::text),
     ('ExclusiveLock'::text, 'RowExclusiveLock'::text),
     ('ExclusiveLock'::text, 'ShareUpdateExclusiveLock'::text),
     ('ExclusiveLock'::text, 'ShareLock'::text),
     ('ExclusiveLock'::text, 'ShareRowExclusiveLock'::text),
     ('ExclusiveLock'::text, 'ExclusiveLock'::text),
     ('ExclusiveLock'::text, 'AccessExclusiveLock'::text),
     ('AccessExclusiveLock'::text, 'AccessShareLock'::text),
     ('AccessExclusiveLock'::text, 'RowShareLock'::text),
     ('AccessExclusiveLock'::text, 'RowExclusiveLock'::text),
     ('AccessExclusiveLock'::text, 'ShareUpdateExclusiveLock'::text),
     ('AccessExclusiveLock'::text, 'ShareLock'::text),
     ('AccessExclusiveLock'::text, 'ShareRowExclusiveLock'::text),
     ('AccessExclusiveLock'::text, 'ExclusiveLock'::text),
     ('AccessExclusiveLock'::text, 'AccessExclusiveLock'::text)
  )
, LOCK AS (
  SELECT pid,
     virtualtransaction,
     granted,
     mode,
    (locktype,
     CASE locktype
       WHEN 'relation'      THEN format('db:%s;rel:%s',datname,relation::regclass::text)
       WHEN 'extend'        THEN format('db:%s;rel:%s', datname, relation::regclass::text)
       WHEN 'page'          THEN format('db:%s;rel:%s;page#%s', datname, relation::regclass::text, page::text)
       WHEN 'tuple'         THEN format('db:%s;rel:%s;page#%s;tuple#%s', datname, relation::regclass::text, page::text, tuple::text)
       WHEN 'transactionid' THEN transactionid::text
       WHEN 'virtualxid'    THEN virtualxid::text
       WHEN 'object'        THEN format('class:%s;objid:%s;col#%s',classid::regclass::text,objid,objsubid)
       ELSE                      format('db:%I',datname) -- userlock and advisory
     END::text) AS target
  FROM pg_catalog.pg_locks
  LEFT JOIN pg_catalog.pg_database ON (pg_database.oid = pg_locks.DATABASE)
  )
, acquired_lock AS (
  SELECT
    pid,
    array_agg(concat(mode,target)) AS locks_acquired
  FROM LOCK
  WHERE granted
  GROUP BY pid
  )
, waiting_lock AS (
  SELECT
    blocker.pid                         AS blocker_pid,
    blocked.pid                         AS pid,
    concat(blocked.mode,blocked.target) AS lock_target
  FROM LOCK blocker
  JOIN LOCK blocked
    ON ( NOT blocked.granted
     AND blocker.granted
     AND blocked.pid != blocker.pid
     AND blocked.target IS NOT DISTINCT FROM blocker.target)
  JOIN lock_composite c ON (c.requested = blocked.mode AND c.current = blocker.mode)
  )
, blocking_lock AS (
  SELECT
    ARRAY[date_part('epoch', query_start)::int, pid] AS seq,
     0::int AS depth,
    -1::int AS blocker_pid,
    pid,
    concat('Connect: ',usename,' ',datname,' ',host(client_addr),':',client_port
      , E'\nSQL: ',REPLACE(substr(coalesce(query,'N/A'), 1, 60), E'\n', ' ')
      , E'\nAcquired:\n  ',array_to_string(locks_acquired,E'\n  ')
    ) AS lock_info,
    concat(to_char(query_start, CASE WHEN age(query_start) > '24h' THEN 'Day DD Mon' ELSE 'HH24:MI:SS' END),E' started\n'
          ,CASE WHEN waiting THEN 'waiting' ELSE state END,E'\n'
          ,date_trunc('second',age(now(),query_start)),' ago'
    ) AS lock_state
  FROM acquired_lock blocker
  LEFT JOIN pg_stat_activity act USING (pid)
  WHERE EXISTS
   (SELECT 'x' FROM waiting_lock blocked WHERE blocked.blocker_pid = blocker.pid)
UNION ALL
  SELECT
    blocker.seq || blocked.pid,
    blocker.depth + 1,
    blocker.pid,
    blocked.pid,
    concat('Connect: ',usename,' ',datname,' ',host(client_addr),':',client_port
      , E'\nSQL: ',REPLACE(substr(coalesce(query,'N/A'), 1, 60), E'\n', ' ')
      , E'\nWaiting: ',blocked.lock_target
      , CASE WHEN locks_acquired IS NOT NULL
             THEN E'\nAcquired:\n  '||array_to_string(locks_acquired,E'\n  ') END
    ) AS lock_info,
    concat(to_char(query_start, CASE WHEN age(query_start) > '24h' THEN 'Day DD Mon' ELSE 'HH24:MI:SS' END),E' started\n'
          ,CASE WHEN waiting THEN 'waiting' ELSE state END,E'\n'
          ,date_trunc('second',age(now(),query_start)),' ago'
    ) AS lock_state
  FROM blocking_lock blocker
  JOIN waiting_lock blocked
    ON (blocked.blocker_pid = blocker.pid)
  LEFT JOIN pg_stat_activity act ON (act.pid = blocked.pid)
  LEFT JOIN acquired_lock acq ON (acq.pid = blocked.pid)
  WHERE blocker.depth < 5
  )
SELECT concat(lpad('=> ', 4*depth, ' '),pid::text) AS "PID"
, lock_info AS "Lock Info"
, lock_state AS "State"
FROM blocking_lock
ORDER BY seq;
