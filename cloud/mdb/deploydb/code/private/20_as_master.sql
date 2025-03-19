CREATE OR REPLACE FUNCTION code._as_master(
    master       deploy.masters,
    deploy_group deploy.groups,
    aliases      text[]
) RETURNS code.master AS $$
SELECT master.master_id,
       master.fqdn,
       coalesce(aliases, '{}'::text[]),
       deploy_group.name,
       master.is_open,
       master.description,
       master.created_at,
       master.alive_check_at,
       master.is_alive;
$$ LANGUAGE SQL IMMUTABLE;