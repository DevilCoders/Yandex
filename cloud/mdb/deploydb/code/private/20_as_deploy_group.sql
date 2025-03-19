CREATE OR REPLACE FUNCTION code._as_deploy_group(
    deploy.groups
) RETURNS code.deploy_group AS $$
SELECT $1.group_id, $1.name;
$$ LANGUAGE SQL IMMUTABLE;
