CREATE OR REPLACE FUNCTION code.get_minion(
    i_fqdn text
)
RETURNS SETOF code.minion AS $$
SELECT (code._as_minion(minions, masters, groups)).*
  FROM deploy.masters
  JOIN deploy.groups
 USING (group_id)
  JOIN deploy.minions
 USING (master_id)
 WHERE minions.fqdn = i_fqdn;
$$ LANGUAGE SQL STABLE;
