CREATE OR REPLACE FUNCTION code._as_minion(
    minion       deploy.minions,
    master       deploy.masters,
    deploy_group deploy.groups
) RETURNS code.minion AS $$
SELECT $1.minion_id,
       $1.fqdn,
       deploy_group.name,
       master.fqdn,
       $1.auto_reassign,
       $1.created_at,
       $1.updated_at,
       $1.register_until,
       $1.pub_key,
       $1.pub_key IS NOT NULL,
       $1.deleted;
$$ LANGUAGE SQL IMMUTABLE;
