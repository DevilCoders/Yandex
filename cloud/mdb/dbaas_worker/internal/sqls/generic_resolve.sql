SELECT h.fqdn,
       h.geo,
       r.name AS region_name,
       h.space_limit,
       h.subnet_id,
       f.platform_id,
       f.cpu_guarantee,
       f.cpu_limit,
       f.gpu_limit,
       f.memory_guarantee,
       f.memory_limit,
       f.network_guarantee,
       f.network_limit,
       f.io_limit,
       f.io_cores_limit,
       f.name AS flavor,
       f.cloud_provider_flavor_name,
       f.arch,
       h.disk_type_id,
       f.vtype,
       h.vtype_id,
       h.assign_public_ip,
       h.subcid,
       h.shard_id,
       s.name AS shard_name,
       h.roles::text[],
       h.environment,
       h.host_group_ids,
       dt.cloud_provider_disk_type,
       h.cluster_type
FROM code.get_hosts_by_cid(
       i_cid        => %(cid)s,
       i_visibility => 'all') h
JOIN dbaas.flavors f
  ON f.id = h.flavor_id
JOIN dbaas.disk_type dt
  ON dt.disk_type_ext_id = h.disk_type_id
LEFT JOIN dbaas.shards s
  ON s.shard_id = h.shard_id
JOIN dbaas.geo g
  ON h.geo = g.name
LEFT JOIN dbaas.regions r
USING (region_id)
ORDER BY h.fqdn
