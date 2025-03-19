CREATE OR REPLACE FUNCTION code.get_master(
    i_fqdn text
)
RETURNS SETOF code.master AS $$
SELECT (code._as_master(
    masters,
    groups,
    ARRAY(SELECT alias
           FROM deploy.master_aliases
          WHERE master_aliases.master_id = masters.master_id))).*
  FROM deploy.groups
  JOIN deploy.masters
 USING (group_id)
 WHERE masters.fqdn = i_fqdn;
$$ LANGUAGE SQL STABLE;