
SELECT code.set_default_versions(
        i_cid           => %(cid)s,
        i_subcid        => %(subcid)s,
        i_shard_id      => %(shard_id)s,
        i_ctype         => %(ctype)s::dbaas.cluster_type,
        i_env           => %(env)s::dbaas.env_type,
        i_major_version => %(major_version)s,
        i_edition       => %(edition)s,
        i_rev           => %(rev)s
    )
