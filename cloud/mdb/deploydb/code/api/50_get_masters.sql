CREATE OR REPLACE FUNCTION code.get_masters(
    i_limit             bigint,
    i_last_master_id    bigint DEFAULT NULL
) RETURNS SETOF code.master AS $$
SELECT (code._as_master(masters, groups, a.aliases)).*
  FROM deploy.groups
  JOIN deploy.masters
 USING (group_id)
 LEFT JOIN (
    SELECT master_id, array_agg(alias) AS aliases
      FROM deploy.master_aliases
     GROUP BY master_id) a
 USING (master_id)
 WHERE (i_last_master_id IS NULL OR masters.master_id > i_last_master_id)
 ORDER BY masters.master_id ASC
 LIMIT i_limit;
$$ LANGUAGE SQL STABLE;