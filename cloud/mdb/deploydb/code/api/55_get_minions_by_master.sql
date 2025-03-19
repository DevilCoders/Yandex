CREATE OR REPLACE FUNCTION code.get_minions_by_master(
    i_master_fqdn       text,
    i_limit             bigint,
    i_last_minion_id    bigint DEFAULT NULL
)
RETURNS SETOF code.minion AS $$
SELECT (code._as_minion(minions, masters, groups)).*
  FROM deploy.masters
  JOIN deploy.groups
 USING (group_id)
  JOIN deploy.minions
 USING (master_id)
 WHERE masters.fqdn = i_master_fqdn
   AND (i_last_minion_id IS NULL OR minions.minion_id > i_last_minion_id)
 ORDER BY minions.minion_id ASC
 LIMIT i_limit;
$$ LANGUAGE SQL STABLE;