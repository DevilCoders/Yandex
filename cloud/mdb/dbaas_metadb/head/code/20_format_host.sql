CREATE OR REPLACE FUNCTION code.format_host(
    h dbaas.hosts,
    c dbaas.clusters,
    s dbaas.subclusters,
    g dbaas.geo,
    d dbaas.disk_type,
    f dbaas.flavors
) RETURNS code.host AS $$
SELECT
    h.subcid,
    h.shard_id,
    h.space_limit,
    h.flavor,
    h.subnet_id,
    g.name,
    d.disk_type_ext_id AS disk_type_id,
    h.fqdn,
    f.vtype,
    h.vtype_id,
    s.roles,
    s.name,
    h.assign_public_ip,
    c.env,
    f.name,
    h.created_at,
    c.host_group_ids,
    c.type as cluster_type,
    h.is_in_user_project_id;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.format_host(
    h dbaas.hosts
) RETURNS code.host AS $$
SELECT fmt.*
  FROM dbaas.subclusters s,
       dbaas.clusters c,
       dbaas.geo g,
       dbaas.disk_type d,
       dbaas.flavors f,
       code.format_host(h, c, s, g, d, f) fmt
 WHERE s.subcid = (h).subcid
   AND c.cid = s.cid
   AND g.geo_id = (h).geo_id
   AND d.disk_type_id = (h).disk_type_id
   AND f.id = (h).flavor;
$$ LANGUAGE SQL STABLE;

DROP FUNCTION IF EXISTS code.format_host(
    h dbaas.hosts_revs,
    c dbaas.clusters,
    s dbaas.subclusters_revs,
    g dbaas.geo,
    d dbaas.disk_type,
    f dbaas.flavors
);

CREATE OR REPLACE FUNCTION code.format_host(
    h dbaas.hosts_revs,
    c dbaas.clusters,
    cr dbaas.clusters_revs,
    s dbaas.subclusters_revs,
    g dbaas.geo,
    d dbaas.disk_type,
    f dbaas.flavors
) RETURNS code.host AS $$
SELECT
    h.subcid,
    h.shard_id,
    h.space_limit,
    h.flavor,
    h.subnet_id,
    g.name,
    d.disk_type_ext_id AS disk_type_id,
    h.fqdn,
    f.vtype,
    h.vtype_id,
    s.roles,
    s.name,
    h.assign_public_ip,
    c.env,
    f.name,
    h.created_at,
    cr.host_group_ids,
    c.type as cluster_type,
    h.is_in_user_project_id;
$$ LANGUAGE SQL IMMUTABLE;
