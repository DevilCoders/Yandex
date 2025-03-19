CREATE OR REPLACE FUNCTION code.get_group(i_name text)
RETURNS SETOF code.deploy_group AS $$
SELECT (code._as_deploy_group(groups)).*
  FROM deploy.groups
 WHERE name = i_name;
$$ LANGUAGE SQL STABLE;