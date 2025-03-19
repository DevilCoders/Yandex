SELECT
    h.subcid,
    h.shard_id,
    s.name AS shard_name,
    flavor_id AS flavor,
    space_limit,
    geo,
    fqdn,
    f.name,
    h.vtype,
    vtype_id,
    disk_type_id,
    subnet_id,
    assign_public_ip,
    roles::text[] AS roles,
    subcluster_name
FROM code.get_host(%(fqdn)s) h
JOIN dbaas.flavors f
  ON (f.id = h.flavor_id)
LEFT JOIN dbaas.shards s
  ON s.shard_id = h.shard_id
