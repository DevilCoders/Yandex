CREATE OR REPLACE FUNCTION code.get_groups(
  i_limit           bigint,
  i_last_group_id   bigint DEFAULT NULL,
  i_ascending       boolean DEFAULT true
) RETURNS SETOF code.deploy_group AS $$
SELECT (code._as_deploy_group(groups)).*
  FROM deploy.groups
  WHERE (i_last_group_id IS NULL OR group_id > i_last_group_id)
		ORDER BY group_id * CASE WHEN i_ascending THEN 1 ELSE -1 END ASC
		LIMIT i_limit;
$$ LANGUAGE SQL STABLE;
