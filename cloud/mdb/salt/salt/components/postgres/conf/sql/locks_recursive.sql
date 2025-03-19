WITH RECURSIVE
     c(requested, current) AS
       ( VALUES
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
       ),
     l AS
       (
         SELECT
             (locktype,DATABASE,relation::regclass::text,page,tuple,virtualxid,transactionid,classid,objid,objsubid) AS target,
             virtualtransaction,
             pid,
             mode,
             granted
           FROM pg_catalog.pg_locks
       ),
     t AS
       (
         SELECT
             blocker.target  AS blocker_target,
             blocker.pid     AS blocker_pid,
             blocker.mode    AS blocker_mode,
             blocked.target  AS target,
             blocked.pid     AS pid,
             blocked.mode    AS mode
           FROM l blocker
           JOIN l blocked
             ON ( NOT blocked.granted
              AND blocker.granted
              AND blocked.pid != blocker.pid
              AND blocked.target IS NOT DISTINCT FROM blocker.target)
           JOIN c ON (c.requested = blocked.mode AND c.current = blocker.mode)
       ),
     r AS
       (
         SELECT
             blocker_target,
             blocker_pid,
             blocker_mode,
             '1'::int        AS depth,
             target,
             pid,
             mode,
             blocker_pid::text || ',' || pid::text AS seq
           FROM t
         UNION ALL
         SELECT
             blocker.blocker_target,
             blocker.blocker_pid,
             blocker.blocker_mode,
             blocker.depth + 1,
             blocked.target,
             blocked.pid,
             blocked.mode,
             blocker.seq || ',' || blocked.pid::text
           FROM r blocker
           JOIN t blocked
             ON (blocked.blocker_pid = blocker.pid)
           WHERE blocker.depth < 1000
       )
SELECT * FROM r
  ORDER BY seq;
