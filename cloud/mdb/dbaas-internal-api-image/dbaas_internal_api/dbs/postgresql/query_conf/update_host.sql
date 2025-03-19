SELECT
    subcid,
    shard_id,
    space_limit,
    flavor_id AS flavor,
    geo,
    fqdn
FROM code.update_host(
    i_fqdn             => %(fqdn)s,
    i_space_limit      => %(space_limit)s,
    i_flavor_id        => %(flavor_id)s,
    i_cid              => %(cid)s,
    i_rev              => %(rev)s,
    i_assign_public_ip => %(assign_public_ip)s
)