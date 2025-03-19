SELECT pid, datname, usename, sa.state, xact_start, application_name
  FROM pg_locks l
  JOIN pg_stat_activity sa
 USING (pid)
 WHERE l.locktype = 'advisory' AND l.objid=%(geo_id)s AND l.granted
